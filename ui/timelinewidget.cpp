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
#include "panels/viewer.h"
#include "project/undo.h"
#include "ui_timeline.h"
#include "mainwindow.h"

#include "effects/effect.h"
#include "effects/transition.h"
#include "effects/video/solideffect.h"

#include <QPainter>
#include <QColor>
#include <QDebug>
#include <QMouseEvent>
#include <QObject>
#include <QVariant>
#include <QPoint>
#include <QMenu>
#include <QMessageBox>
#include <QtMath>
#include <QScrollBar>
#include <QMimeData>
#include <QToolTip>

#define MAX_TEXT_WIDTH 20

TimelineWidget::TimelineWidget(QWidget *parent) : QWidget(parent) {
	bottom_align = false;
    track_resizing = false;
    setMouseTracking(true);

    setAcceptDrops(true);

    setContextMenuPolicy(Qt::CustomContextMenu);
    connect(this, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(show_context_menu(const QPoint&)));

    tooltip_timer.setInterval(500);
    connect(&tooltip_timer, SIGNAL(timeout()), this, SLOT(tooltip_timer_timeout()));
}

void TimelineWidget::right_click_ripple() {
	bool can_ripple = true;

	// validate the ripple
	for (int i=0;i<sequence->clips.size();i++) {
		Clip* c = sequence->clips.at(i);
		if (c != NULL && c->timeline_in > rc_ripple_min) {
			for (int j=0;j<sequence->clips.size();j++) {
				Clip* cc = sequence->clips.at(j);
				if (cc != NULL && cc->track == c->track) {
					if (cc->timeline_in < rc_ripple_min && cc->timeline_out == c->timeline_in) {
						can_ripple = false;
						break;
					} else if (cc->timeline_out < c->timeline_in) {
						long validator = (c->timeline_in - (rc_ripple_max - rc_ripple_min)) - cc->timeline_out;
						if (validator < 0) {
							rc_ripple_min = cc->timeline_out;
							rc_ripple_max = c->timeline_in;
						}
					}
				}
			}
		}
	}

	if (can_ripple) {
		undo_stack.push(new RippleCommand(sequence, rc_ripple_min, rc_ripple_min - rc_ripple_max));
		panel_timeline->repaint_timeline(true);
	}
}

void TimelineWidget::show_context_menu(const QPoint& pos) {
	// hack because sometimes right clicking doesn't trigger mouseReleaseEvent
	panel_timeline->rect_select_init = false;
	panel_timeline->rect_select_proc = false;

    QMenu menu(this);

	QAction* undoAction = menu.addAction("&Undo");
	QAction* redoAction = menu.addAction("&Redo");
	connect(undoAction, SIGNAL(triggered(bool)), mainWindow, SLOT(undo()));
	connect(redoAction, SIGNAL(triggered(bool)), mainWindow, SLOT(redo()));
	undoAction->setEnabled(undo_stack.canUndo());
	redoAction->setEnabled(undo_stack.canRedo());
	menu.addSeparator();

	if (sequence->selections.size() == 0) {
		panel_timeline->cursor_frame = panel_timeline->getTimelineFrameFromScreenPoint(pos.x());
		panel_timeline->cursor_track = getTrackFromScreenPoint(pos.y());

		bool can_ripple_delete = true;
		bool at_end_of_sequence = true;
		rc_ripple_min = LONG_MIN;
		rc_ripple_max = LONG_MAX;

		for (int i=0;i<sequence->clips.size();i++) {
			Clip* c = sequence->clips.at(i);
			if (c != NULL) {
				if (c->timeline_in > panel_timeline->cursor_frame || c->timeline_out > panel_timeline->cursor_frame) {
					at_end_of_sequence = false;
				}
				if (c->track == panel_timeline->cursor_track) {
					if (c->timeline_in <= panel_timeline->cursor_frame && c->timeline_out >= panel_timeline->cursor_frame) {
						can_ripple_delete = false;
						break;
					} else if (c->timeline_out < panel_timeline->cursor_frame) {
						rc_ripple_min = qMax(rc_ripple_min, c->timeline_out);
					} else if (c->timeline_in > panel_timeline->cursor_frame) {
						rc_ripple_max = qMin(rc_ripple_max, c->timeline_in);
					}
				}
			}
		}

		if (can_ripple_delete && !at_end_of_sequence) {
			QAction* ripple_delete_action = menu.addAction("&Ripple Delete");
			connect(ripple_delete_action, SIGNAL(triggered(bool)), this, SLOT(right_click_ripple()));
		}

		menu.addAction("Sequence settings coming soon...");
	} else {
		QAction* cutAction = menu.addAction("C&ut");
		QAction* copyAction = menu.addAction("Cop&y");
		QAction* pasteAction = menu.addAction("&Paste");
		menu.addSeparator();
		QAction* speedAction = menu.addAction("&Speed/Duration");
		connect(speedAction, SIGNAL(triggered(bool)), mainWindow, SLOT(openSpeedDialog()));
        QAction* autoscaleAction = menu.addAction("Auto-s&cale");
        autoscaleAction->setCheckable(true);
        connect(autoscaleAction, SIGNAL(triggered(bool)), this, SLOT(toggle_autoscale()));

        for (int i=0;i<sequence->clips.size();i++) {
            Clip* c = sequence->clips.at(i);
            if (c != NULL && panel_timeline->is_clip_selected(c, true)) {
                autoscaleAction->setChecked(c->autoscale);
                break;
            }
        }
	}

    menu.exec(mapToGlobal(pos));
}

void TimelineWidget::toggle_autoscale() {
    SetAutoscaleAction* action = new SetAutoscaleAction();
    for (int i=0;i<sequence->clips.size();i++) {
        Clip* c = sequence->clips.at(i);
        if (c != NULL && panel_timeline->is_clip_selected(c, true)) {
            action->clips.append(c);
        }
    }
    if (action->clips.size() > 0) {
        undo_stack.push(action);
    } else {
        delete action;
    }
}

void TimelineWidget::tooltip_timer_timeout() {
    if (sequence != NULL) {
        Clip* c = sequence->clips.at(tooltip_clip);
        if (c != NULL) {
            QToolTip::showText(QCursor::pos(),
                        c->name
                               + "\nStart: " + frame_to_timecode(c->timeline_in, config.timecode_view, sequence->frame_rate)
                               + "\nEnd: " + frame_to_timecode(c->timeline_out, config.timecode_view, sequence->frame_rate)
                               + "\nDuration: " + frame_to_timecode(c->getLength(), config.timecode_view, sequence->frame_rate));
        }
    }
	tooltip_timer.stop();
}

bool same_sign(int a, int b) {
	return (a < 0) == (b < 0);
}

void TimelineWidget::dragEnterEvent(QDragEnterEvent *event) {
    if (event->source() == panel_project->source_table) {
        event->accept();

        panel_timeline->video_ghosts = false;
        panel_timeline->audio_ghosts = false;
        QPoint pos = event->pos();
        QList<QTreeWidgetItem*> items = panel_project->source_table->selectedItems();
        long entry_point;
        if (sequence == NULL) {
            // if no sequence, we're going to create a new one using the clips as a reference
            entry_point = 0;

            // shitty hardcoded default values
            predicted_video_width = 1920;
            predicted_video_height = 1080;
            predicted_new_frame_rate = 29.97;
            predicted_audio_freq = 48000;
            predicted_audio_layout = 3;

            bool got_video_values = false;
            bool got_audio_values = false;
            for (int i=0;i<items.size();i++) {
                QTreeWidgetItem* item = items.at(i);
                switch (get_type_from_tree(item)) {
                case MEDIA_TYPE_FOOTAGE:
                {
					Media* m = get_footage_from_tree(item);
                    if (m->ready) {
                        if (!got_video_values) {
                            for (int j=0;j<m->video_tracks.size();j++) {
                                MediaStream* ms = m->video_tracks.at(j);
                                predicted_video_width = ms->video_width;
                                predicted_video_height = ms->video_height;
                                if (ms->video_frame_rate != 0) {
                                    predicted_new_frame_rate = ms->video_frame_rate;

                                    // only break with a decent frame rate, otherwise there may be a better candidate
                                    got_video_values = true;
                                    break;
                                }
                            }
                        }
                        if (!got_audio_values) {
                            for (int j=0;j<m->audio_tracks.size();j++) {
                                MediaStream* ms = m->audio_tracks.at(j);
                                predicted_audio_freq = ms->audio_frequency;
                                got_audio_values = true;
                                break;
                            }
                        }
                    }
                }
                    break;
                case MEDIA_TYPE_SEQUENCE:
                {
                    Sequence* s = get_sequence_from_tree(item);
                    predicted_video_width = s->width;
                    predicted_video_height = s->height;
                    predicted_new_frame_rate = s->frame_rate;
                    predicted_audio_freq = s->audio_frequency;
                    predicted_audio_layout = s->audio_layout;

                    got_video_values = true;
                    got_audio_values = true;
                }
                    break;
                }
                if (got_video_values && got_audio_values) break;
            }
        } else {
			entry_point = panel_timeline->getTimelineFrameFromScreenPoint(pos.x());
			panel_timeline->drag_frame_start = entry_point + panel_timeline->getTimelineFrameFromScreenPoint(50);
            panel_timeline->drag_track_start = (bottom_align) ? -1 : 0;
            predicted_new_frame_rate = sequence->frame_rate;
        }
        for (int i=0;i<items.size();i++) {
            QTreeWidgetItem* item = items.at(i);
            int item_type = get_type_from_tree(item);
            bool can_import = true;

            Media* m = NULL;
            Sequence* s = NULL;
            void* media = NULL;
            long sequence_length;

            switch (item_type) {
            case MEDIA_TYPE_FOOTAGE:
				m = get_footage_from_tree(item);
                media = m;
                can_import = m->ready;
                break;
            case MEDIA_TYPE_SEQUENCE:
                s = get_sequence_from_tree(item);
                sequence_length = s->getEndFrame();
                if (sequence != NULL) sequence_length = refactor_frame_number(sequence_length, s->frame_rate, sequence->frame_rate);
                media = s;
                can_import = (s != sequence && sequence_length != 0);
                break;
            default:
                can_import = false;
            }

            if (can_import) {
                Ghost g;
                g.clip = -1;
                g.media_type = item_type;
                g.trimming = false;
                g.old_clip_in = g.clip_in = 0;
                g.media = media;
                g.in = entry_point;
				g.transition = NULL;

                // is video source a still image?
                switch (item_type) {
                case MEDIA_TYPE_FOOTAGE:
                    if (m->video_tracks.size() > 0 && m->video_tracks[0]->infinite_length && m->audio_tracks.size() == 0) {
                        g.out = g.in + 100;
                    } else {
                        g.out = entry_point + m->get_length_in_frames(predicted_new_frame_rate);
                    }

                    for (int j=0;j<m->audio_tracks.size();j++) {
                        g.track = j;
                        g.media_stream = m->audio_tracks.at(j)->file_index;
                        panel_timeline->ghosts.append(g);
                        panel_timeline->audio_ghosts = true;
                    }
                    for (int j=0;j<m->video_tracks.size();j++) {
                        g.track = -1-j;
                        g.media_stream = m->video_tracks.at(j)->file_index;
                        panel_timeline->ghosts.append(g);
                        panel_timeline->video_ghosts = true;
                    }
                    break;
                case MEDIA_TYPE_SEQUENCE:
                    g.out = entry_point + sequence_length;

                    g.track = -1;
                    panel_timeline->ghosts.append(g);
                    g.track = 0;
                    panel_timeline->ghosts.append(g);

                    panel_timeline->video_ghosts = true;
                    panel_timeline->audio_ghosts = true;
                    break;
                }
                entry_point = g.out;
            }
        }
        for (int i=0;i<panel_timeline->ghosts.size();i++) {
            Ghost& g = panel_timeline->ghosts[i];
            g.old_in = g.in;
            g.old_out = g.out;
            g.old_track = g.track;
        }
        panel_timeline->importing = true;
    } else if (config.enable_drag_files_to_timeline && event->mimeData()->hasUrls()) {
        // TODO for this to work, we need a way to abort PreviewGenerator

		event->accept();
        qDebug() << "TODO get data for:";
        QList<QUrl> urls = event->mimeData()->urls();
        if (!urls.isEmpty()) {
            for (int i=0;i<urls.size();i++) {
                qDebug() << (urls.at(i).toLocalFile());
            }
        }
    }
}

void TimelineWidget::dragMoveEvent(QDragMoveEvent *event) {
    if (sequence != NULL && panel_timeline->importing) {
		QPoint pos = event->pos();
        update_ghosts(pos);
		panel_timeline->repaint_timeline(false);
	}
}

void TimelineWidget::dragLeaveEvent(QDragLeaveEvent*) {
    if (sequence != NULL && panel_timeline->importing) {
		panel_timeline->ghosts.clear();
		panel_timeline->importing = false;
		panel_timeline->repaint_timeline(false);
	}
}

void TimelineWidget::dropEvent(QDropEvent* event) {
    if (panel_timeline->importing && panel_timeline->ghosts.size() > 0) {
        event->accept();

        ComboAction* ca = new ComboAction();

        QVector<Clip*> added_clips;

        Sequence* s = sequence;

        // if we're dropping into nothing, create a new sequences based on the clip being dragged
        if (s == NULL) {
            s = new Sequence();

            // dumb hardcoded default values, should be settable somewhere
            s->name = panel_project->get_next_sequence_name();
            s->width = predicted_video_width;
            s->height = predicted_video_height;
            s->frame_rate = predicted_new_frame_rate;
            s->audio_frequency = predicted_audio_freq;
            s->audio_layout = predicted_audio_layout;

            panel_project->new_sequence(ca, s, true, NULL);
        } else {
            // delete areas before adding
            QVector<Selection> delete_areas;
            for (int i=0;i<panel_timeline->ghosts.size();i++) {
                const Ghost& g = panel_timeline->ghosts.at(i);
                Selection sel;
                sel.in = g.in;
                sel.out = g.out;
                sel.track = g.track;
                delete_areas.append(sel);
            }
            panel_timeline->delete_areas_and_relink(ca, delete_areas);
        }

        // add clips
		for (int i=0;i<panel_timeline->ghosts.size();i++) {
            const Ghost& g = panel_timeline->ghosts.at(i);

            Clip* c = new Clip(s);
            c->media = g.media;
            c->media_type = g.media_type;
            c->media_stream = g.media_stream;
            c->timeline_in = g.in;
            c->timeline_out = g.out;
            c->clip_in = g.clip_in;
			c->track = g.track;
            if (c->media_type == MEDIA_TYPE_FOOTAGE) {
                Media* m = static_cast<Media*>(c->media);
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
            } else if (c->media_type == MEDIA_TYPE_SEQUENCE) {
                // sequence (red?ish?)
                c->color_r = 192;
                c->color_g = 128;
                c->color_b = 128;

				Sequence* media = static_cast<Sequence*>(c->media);
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

            if (c->track < 0) {
                // add default video effects
                c->effects.append(create_effect(VIDEO_TRANSFORM_EFFECT, c));
            } else {
                // add default audio effects
                c->effects.append(create_effect(AUDIO_VOLUME_EFFECT, c));
                c->effects.append(create_effect(AUDIO_PAN_EFFECT, c));
            }
        }

		panel_timeline->ghosts.clear();
		panel_timeline->importing = false;
        panel_timeline->snapped = false;

        undo_stack.push(ca);

        setFocus();

		panel_timeline->repaint_timeline(true);
	}
}

void TimelineWidget::mouseDoubleClickEvent(QMouseEvent *event) {
    if (panel_timeline->tool == TIMELINE_TOOL_EDIT) {
        int clip_index = getClipIndexFromCoords(panel_timeline->cursor_frame, panel_timeline->cursor_track);
        if (clip_index >= 0) {
            Clip* clip = sequence->clips.at(clip_index);
			if (!(event->modifiers() & Qt::ShiftModifier)) sequence->selections.clear();
            Selection s;
            s.in = clip->timeline_in;
            s.out = clip->timeline_out;
            s.track = clip->track;
			sequence->selections.append(s);
			panel_timeline->repaint_timeline(false);
        }
    }
}

bool isLiveEditing() {
	return (panel_timeline->tool == TIMELINE_TOOL_EDIT || panel_timeline->tool == TIMELINE_TOOL_RAZOR || panel_timeline->creating);
}

void TimelineWidget::mousePressEvent(QMouseEvent *event) {
    if (sequence != NULL) {
		int tool = panel_timeline->tool;
		if (event->button() == Qt::RightButton) {
			tool = TIMELINE_TOOL_MENU;
			panel_timeline->creating = false;
		}

		QPoint pos = event->pos();
		if (isLiveEditing()) {
			panel_timeline->drag_frame_start = panel_timeline->cursor_frame;
			panel_timeline->drag_track_start = panel_timeline->cursor_track;
		} else {
			panel_timeline->drag_frame_start = panel_timeline->getTimelineFrameFromScreenPoint(pos.x());
			panel_timeline->drag_track_start = getTrackFromScreenPoint(pos.y());
		}

		int clip_index = panel_timeline->trim_target;
		if (clip_index == -1) clip_index = getClipIndexFromCoords(panel_timeline->drag_frame_start, panel_timeline->drag_track_start);

		bool shift = (event->modifiers() & Qt::ShiftModifier);
		bool alt = (event->modifiers() & Qt::AltModifier);

		if (shift) {
			panel_timeline->selection_offset = sequence->selections.size();
		} else {
			panel_timeline->selection_offset = 0;
		}

		if (panel_timeline->creating) {
			int comp = 0;
			switch (panel_timeline->creatingObject) {
			case ADD_OBJ_TITLE:
			case ADD_OBJ_SOLID:
			case ADD_OBJ_BARS:
				comp = -1;
				break;
			case ADD_OBJ_TONE:
			case ADD_OBJ_NOISE:
				comp = 1;
				break;
			}

			if ((panel_timeline->drag_track_start < 0) == (comp < 0)) {
				Ghost g;
				g.in = g.old_in = g.out = g.old_out = panel_timeline->drag_frame_start;
				g.track = g.old_track = panel_timeline->drag_track_start;
				g.transition = NULL;
				g.clip = -1;
				g.trimming = true;
				g.trim_in = false;
				panel_timeline->ghosts.append(g);

				panel_timeline->moving_init = true;
				panel_timeline->moving_proc = true;
			}
		} else {
			switch (tool) {
			case TIMELINE_TOOL_POINTER:
			case TIMELINE_TOOL_RIPPLE:
			case TIMELINE_TOOL_SLIP:
			case TIMELINE_TOOL_ROLLING:
			case TIMELINE_TOOL_SLIDE:
			case TIMELINE_TOOL_MENU:
			{
				if (track_resizing && tool != TIMELINE_TOOL_MENU) {
					track_resize_mouse_cache = event->pos().y();
					panel_timeline->moving_init = true;
				} else {
					if (clip_index >= 0) {
						Clip* clip = sequence->clips.at(clip_index);
						if (clip != NULL) {
							if (panel_timeline->is_clip_selected(clip, true)) {
								if (shift) {
									panel_timeline->deselect_area(clip->timeline_in, clip->timeline_out, clip->track);

									if (!alt) {
										for (int i=0;i<clip->linked.size();i++) {
											Clip* link = sequence->clips.at(clip->linked.at(i));
											panel_timeline->deselect_area(link->timeline_in, link->timeline_out, link->track);
										}
									}
								} else {
									if (panel_timeline->transition_select != TA_NO_TRANSITION) {
										panel_timeline->deselect_area(clip->timeline_in, clip->timeline_out, clip->track);

										for (int i=0;i<clip->linked.size();i++) {
											Clip* link = sequence->clips.at(clip->linked.at(i));
											panel_timeline->deselect_area(link->timeline_in, link->timeline_out, link->track);
										}

										Selection s;
										s.track = clip->track;
										if (panel_timeline->transition_select == TA_OPENING_TRANSITION && clip->opening_transition != NULL) {
											s.in = clip->timeline_in;
											s.out = clip->timeline_in + clip->opening_transition->length;
										} else if (panel_timeline->transition_select == TA_CLOSING_TRANSITION && clip->closing_transition != NULL) {
											s.in = clip->timeline_out - clip->closing_transition->length;
											s.out = clip->timeline_out;
										}
										sequence->selections.append(s);
									}
								}
							} else {
								// if "shift" is not down
								if (!shift) {
									sequence->selections.clear();
								}

								Selection s;

								s.in = clip->timeline_in;
								s.out = clip->timeline_out;

								if (panel_timeline->transition_select == TA_OPENING_TRANSITION) {
									s.out = clip->timeline_in + clip->opening_transition->length;
								}

								if (panel_timeline->transition_select == TA_CLOSING_TRANSITION) {
									s.in = clip->timeline_out - clip->closing_transition->length;
								}

								s.track = clip->track;
								sequence->selections.append(s);

								if (config.select_also_seeks) {
									panel_timeline->seek(clip->timeline_in);
								}

								// if alt is not down, select links
								if (!alt && panel_timeline->transition_select == TA_NO_TRANSITION) {
									for (int i=0;i<clip->linked.size();i++) {
										Clip* link = sequence->clips.at(clip->linked.at(i));
										if (!panel_timeline->is_clip_selected(link, true)) {
											Selection ss;
											ss.in = link->timeline_in;
											ss.out = link->timeline_out;
											ss.track = link->track;
											sequence->selections.append(ss);
										}
									}
								}
							}
						}

						if (tool != TIMELINE_TOOL_MENU) panel_timeline->moving_init = true;
					} else {
						// if "shift" is not down
						if (!shift) {
							sequence->selections.clear();
						}

						panel_timeline->rect_select_init = true;
					}
					panel_timeline->repaint_timeline(false);
				}
			}
				break;
			case TIMELINE_TOOL_EDIT:
				if (config.edit_tool_also_seeks) panel_timeline->seek(panel_timeline->drag_frame_start);
				panel_timeline->selecting = true;
				break;
			case TIMELINE_TOOL_RAZOR:
			{
				panel_timeline->splitting = true;
				panel_timeline->split_tracks.append(panel_timeline->drag_track_start);
				panel_timeline->repaint_timeline(false);
			}
				break;
			}
		}
    }
}

void TimelineWidget::mouseReleaseEvent(QMouseEvent *event) {
	QToolTip::hideText();
    if (sequence != NULL) {
        bool alt = (event->modifiers() & Qt::AltModifier);
		bool shift = (event->modifiers() & Qt::ShiftModifier);

        if (event->button() == Qt::LeftButton) {
            bool repaint = false;
			bool redraw = false;

			if (panel_timeline->creating) {
				if (panel_timeline->ghosts.size() > 0) {
					const Ghost& g = panel_timeline->ghosts.at(0);

					if (g.in != g.out) {
                        ComboAction* ca = new ComboAction();

						Clip* c = new Clip(sequence);
						c->media = NULL;
						c->timeline_in = qMin(g.in, g.out);
						c->timeline_out = qMax(g.in, g.out);
						c->clip_in = 0;
						c->color_r = 192;
						c->color_g = 192;
						c->color_b = 64;
						c->track = g.track;

						QVector<Clip*> add;
						add.append(c);
                        ca->append(new AddClipCommand(sequence, add));

						if (c->track < 0) {
							// default video effects (before custom effects)
                            c->effects.append(create_effect(VIDEO_TRANSFORM_EFFECT, c));
							c->media_type = MEDIA_TYPE_SOLID;
						}

						switch (panel_timeline->creatingObject) {
						case ADD_OBJ_TITLE:
							c->name = "Title";
                            c->effects.append(create_effect(VIDEO_TEXT_EFFECT, c));
							break;
						case ADD_OBJ_SOLID:
							c->name = "Solid Color";
                            c->effects.append(create_effect(VIDEO_SOLID_EFFECT, c));
							break;
						case ADD_OBJ_BARS:
                        {
							c->name = "Bars";
                            Effect* e = create_effect(VIDEO_SOLID_EFFECT, c);
                            e->row(0)->field(0)->set_combo_index(1);
                            c->effects.append(e);
                        }
							break;
						case ADD_OBJ_TONE:
							c->name = "Tone";
							c->effects.append(create_effect(AUDIO_TONE_EFFECT, c));
							break;
						case ADD_OBJ_NOISE:
							c->name = "Noise";
							c->effects.append(create_effect(AUDIO_NOISE_EFFECT, c));
							break;
						}

						if (c->track >= 0) {
							// default audio effects (after custom effects)
                            c->effects.append(create_effect(AUDIO_VOLUME_EFFECT, c));
                            c->effects.append(create_effect(AUDIO_PAN_EFFECT, c));
							c->media_type = MEDIA_TYPE_TONE;
						}

						Selection s;
						s.in = c->timeline_in;
						s.out = c->timeline_out;
						s.track = c->track;
						QVector<Selection> areas;
						areas.append(s);
                        panel_timeline->delete_areas_and_relink(ca, areas);

                        undo_stack.push(ca);

						redraw = true;

						if (!shift) {
							panel_timeline->creating = false;
						}
					}
				}
			} else if (panel_timeline->moving_proc) {
				repaint = true;
				if (panel_timeline->ghosts.size() > 0) {
					 const Ghost& first_ghost = panel_timeline->ghosts.at(0);

                     ComboAction* ca = new ComboAction();

					 // if we were RIPPLING, move all the clips
					 if (panel_timeline->tool == TIMELINE_TOOL_RIPPLE) {
						 long ripple_length, ripple_point;

						 // ripple_length becomes the length/number of frames we trimmed
						 // ripple point becomes the point to ripple (i.e. the point after or before which we move every clip)
						 if (panel_timeline->trim_in_point) {
							 ripple_length = first_ghost.old_in - panel_timeline->ghosts.at(0).in;
							 ripple_point = first_ghost.old_in;
						 } else {
							 // if we're trimming an out-point
							 ripple_length = first_ghost.old_out - panel_timeline->ghosts.at(0).out;
							 ripple_point = first_ghost.old_out;
						 }
                         QVector<Clip*> ignore_clips;
						 for (int i=0;i<panel_timeline->ghosts.size();i++) {
							 const Ghost& g = panel_timeline->ghosts.at(i);

							 // push rippled clips forward if necessary
							 if (panel_timeline->trim_in_point) {
                                 ignore_clips.append(sequence->clips.at(g.clip));
								 panel_timeline->ghosts[i].in += ripple_length;
								 panel_timeline->ghosts[i].out += ripple_length;
							 }

							 long comp_point = panel_timeline->trim_in_point ? g.old_in : g.old_out;
							 ripple_point = qMin(ripple_point, comp_point);
						 }
						 if (!panel_timeline->trim_in_point) ripple_length = -ripple_length;
                         RippleCommand* rc = new RippleCommand(sequence, ripple_point, ripple_length);
                         rc->ignore = ignore_clips;
                         ca->append(rc);
					 }

					 if (panel_timeline->tool == TIMELINE_TOOL_POINTER
							 && (event->modifiers() & Qt::AltModifier)
							 && panel_timeline->trim_target == -1) { // if holding alt (and not trimming), duplicate rather than move
						 // duplicate clips
						 QVector<int> old_clips;
						 QVector<Clip*> new_clips;
						 QVector<Selection> delete_areas;
						 for (int i=0;i<panel_timeline->ghosts.size();i++) {
							 const Ghost& g = panel_timeline->ghosts.at(i);
							 if (g.old_in != g.in || g.old_out != g.out || g.track != g.old_track || g.clip_in != g.old_clip_in) {
								 // create copy of clip
                                 Clip* c = sequence->clips.at(g.clip)->copy(sequence);

								 c->timeline_in = g.in;
								 c->timeline_out = g.out;
								 c->track = g.track;

								 Selection s;
								 s.in = g.in;
								 s.out = g.out;
								 s.track = g.track;
								 delete_areas.append(s);

								 old_clips.append(g.clip);
								 new_clips.append(c);
							 }
						 }
						 if (new_clips.size() > 0) {
                             panel_timeline->delete_areas_and_relink(ca, delete_areas);

							 // relink duplicated clips
							 panel_timeline->relink_clips_using_ids(old_clips, new_clips);

                             ca->append(new AddClipCommand(sequence, new_clips));
						 }
					 } else {
						 // move clips
						 if (panel_timeline->tool == TIMELINE_TOOL_POINTER || panel_timeline->tool == TIMELINE_TOOL_SLIDE) {
							 QVector<Selection> delete_areas;
							 for (int i=0;i<panel_timeline->ghosts.size();i++) {
								 // step 1 - set clips that are moving to "undeletable" (to avoid step 2 deleting any part of them)
								 const Ghost& g = panel_timeline->ghosts.at(i);

                                 sequence->clips.at(g.clip)->undeletable = true;

								 Selection s;
								 s.in = g.in;
								 s.out = g.out;
								 s.track = g.track;
								 delete_areas.append(s);
							 }
                             panel_timeline->delete_areas_and_relink(ca, delete_areas);
							 for (int i=0;i<panel_timeline->ghosts.size();i++) {
                                 sequence->clips.at(panel_timeline->ghosts[i].clip)->undeletable = false;
							 }
						 }
						 for (int i=0;i<panel_timeline->ghosts.size();i++) {
							 Ghost& g = panel_timeline->ghosts[i];

							 // step 3 - move clips
                             Clip* c = sequence->clips.at(g.clip);
							 if (g.transition == NULL) {
                                 ca->append(new MoveClipAction(c, c->timeline_in + (g.in - g.old_in), c->timeline_out + (g.out - g.old_out), c->clip_in + (g.clip_in - g.old_clip_in), c->track + (g.track - g.old_track)));

								 // adjust transitions if we need to
								 long new_clip_length = (g.out - g.in);
								 if (c->opening_transition != NULL) {
									 long max_open_length = new_clip_length;
									 if (c->closing_transition != NULL && !panel_timeline->trim_in_point) {
										 max_open_length -= c->closing_transition->length;
									 }
									 if (max_open_length <= 0) {
                                         ca->append(new DeleteTransitionCommand(c, TA_OPENING_TRANSITION));
									 } else if (c->opening_transition->length > max_open_length) {
                                         ca->append(new ModifyTransitionCommand(c, TA_OPENING_TRANSITION, max_open_length));
									 }
								 }
								 if (c->closing_transition != NULL) {
									 long max_open_length = new_clip_length;
									 if (c->opening_transition != NULL && panel_timeline->trim_in_point) {
										 max_open_length -= c->opening_transition->length;
									 }
									 if (max_open_length <= 0) {
                                         ca->append(new DeleteTransitionCommand(c, TA_CLOSING_TRANSITION));
									 } else if (c->closing_transition->length > max_open_length) {
                                         ca->append(new ModifyTransitionCommand(c, TA_CLOSING_TRANSITION, max_open_length));
									 }
								 }
							 } else {
								 bool is_opening_transition = (g.transition == c->opening_transition);
								 long new_transition_length = g.out - g.in;
                                 ca->append(new ModifyTransitionCommand(c, is_opening_transition ? TA_OPENING_TRANSITION : TA_CLOSING_TRANSITION, new_transition_length));

								 long clip_length = c->getLength();
								 if (is_opening_transition) {
									 if (g.in != g.old_in) {
										 // if transition is going to make the clip bigger, make the clip bigger
                                         ca->append(new MoveClipAction(c, c->timeline_in + (g.in - g.old_in), c->timeline_out, c->clip_in + (g.clip_in - g.old_clip_in), c->track));
                                         clip_length -= (g.in - g.old_in);
									 }

									 if (c->closing_transition != NULL) {
										 if (new_transition_length == clip_length) {
                                             ca->append(new DeleteTransitionCommand(c, TA_CLOSING_TRANSITION));
										 } else if (new_transition_length > clip_length - c->closing_transition->length) {
                                             ca->append(new ModifyTransitionCommand(c, TA_CLOSING_TRANSITION, clip_length - new_transition_length));
										 }
									 }
								 } else {
									 if (g.out != g.old_out) {
										 // if transition is going to make the clip bigger, make the clip bigger
                                         ca->append(new MoveClipAction(c, c->timeline_in, c->timeline_out + (g.out - g.old_out), c->clip_in, c->track));
										 clip_length += (g.out - g.old_out);
									 }

									 if (c->opening_transition != NULL) {
										 if (new_transition_length == clip_length) {
                                             ca->append(new DeleteTransitionCommand(c, TA_OPENING_TRANSITION));
										 } else if (new_transition_length > clip_length - c->opening_transition->length) {
                                             ca->append(new ModifyTransitionCommand(c, TA_OPENING_TRANSITION, clip_length - new_transition_length));
										 }
									 }
								 }
							 }
						 }
					 }
                     undo_stack.push(ca);
					 redraw = true;
				}
            } else if (panel_timeline->selecting || panel_timeline->rect_select_proc) {
                repaint = true;
            } else if (panel_timeline->splitting) {
                ComboAction* ca = new ComboAction();
                bool split = false;
                for (int i=0;i<panel_timeline->split_tracks.size();i++) {
                    int split_index = getClipIndexFromCoords(panel_timeline->drag_frame_start, panel_timeline->split_tracks.at(i));
                    if (split_index > -1 && panel_timeline->split_clip_and_relink(ca, split_index, panel_timeline->drag_frame_start, !alt)) {
                        split = true;
                    }
                }
                if (split) {
                    undo_stack.push(ca);
					redraw = true;
                } else {
                    delete ca;
                }
                panel_timeline->split_cache.clear();
            }

            // remove duplicate selections
			panel_timeline->clean_up_selections(sequence->selections);

            // destroy all ghosts
            panel_timeline->ghosts.clear();

            // clear split tracks
            panel_timeline->split_tracks.clear();

            panel_timeline->selecting = false;
            panel_timeline->moving_proc = false;
            panel_timeline->moving_init = false;
            panel_timeline->splitting = false;
            panel_timeline->snapped = false;
            panel_timeline->rect_select_init = false;
            panel_timeline->rect_select_proc = false;
            pre_clips.clear();
            post_clips.clear();

			if (redraw) {
				panel_timeline->repaint_timeline(true);
			} else {
				if (repaint) {
					panel_timeline->repaint_timeline(false);
				}
				panel_timeline->update_effect_controls();
			}
        }
    }
}

void TimelineWidget::init_ghosts() {
	for (int i=0;i<panel_timeline->ghosts.size();i++) {
		Ghost& g = panel_timeline->ghosts[i];
        Clip* c = sequence->clips.at(g.clip);

		g.track = g.old_track = c->track;
		g.clip_in = g.old_clip_in = c->clip_in;

		if (g.transition == NULL) {
			// this ghost is for a clip
			g.in = g.old_in = c->timeline_in;
			g.out = g.old_out = c->timeline_out;
			g.ghost_length = g.old_out - g.old_in;
		} else if (g.transition == c->opening_transition) {
			g.in = g.old_in = c->timeline_in;
			g.out = g.old_out = c->timeline_in + c->opening_transition->length;
			g.ghost_length = c->opening_transition->length;
		} else if (g.transition == c->closing_transition) {
			g.in = g.old_in = c->timeline_out - c->closing_transition->length;
			g.out = g.old_out = c->timeline_out;
			g.ghost_length = c->closing_transition->length;
			g.clip_in = g.old_clip_in = c->clip_in + c->getLength() - c->closing_transition->length;
		}

		// used for trim ops
		g.media_length = c->getMaximumLength();
	}
	for (int i=0;i<sequence->selections.size();i++) {
		Selection& s = sequence->selections[i];
		s.old_in = s.in;
		s.old_out = s.out;
		s.old_track = s.track;
	}
}

void TimelineWidget::update_ghosts(QPoint& mouse_pos) {
	int mouse_track = getTrackFromScreenPoint(mouse_pos.y());
	long frame_diff = panel_timeline->getTimelineFrameFromScreenPoint(mouse_pos.x()) - panel_timeline->drag_frame_start;
	int track_diff = ((panel_timeline->tool == TIMELINE_TOOL_SLIDE || panel_timeline->transition_select != TA_NO_TRANSITION) && !panel_timeline->importing) ? 0 : mouse_track - panel_timeline->drag_track_start;
    long validator;

	// first try to snap
	long fm;
    if (panel_timeline->tool != TIMELINE_TOOL_SLIP) {
        // slipping doesn't move the clips so we don't bother snapping for it
        for (int i=0;i<panel_timeline->ghosts.size();i++) {
			Ghost& g = panel_timeline->ghosts[i];
			if (panel_timeline->trim_target == -1 || g.trim_in) {
				fm = g.old_in + frame_diff;
				if (panel_timeline->snap_to_timeline(&fm, true, true, true)) {
					frame_diff = fm - g.old_in;
					break;
				}
			}
			if (panel_timeline->trim_target == -1 || !g.trim_in) {
				fm = g.old_out + frame_diff;
				if (panel_timeline->snap_to_timeline(&fm, true, true, true)) {
					frame_diff = fm - g.old_out;
					break;
				}
			}
        }
    }

    bool clips_are_movable = (panel_timeline->tool == TIMELINE_TOOL_POINTER || panel_timeline->tool == TIMELINE_TOOL_SLIDE || panel_timeline->importing);

    // validate ghosts
    long temp_frame_diff = frame_diff; // cache to see if we change it (thus cancelling any snap)
	for (int i=0;i<panel_timeline->ghosts.size();i++) {
        const Ghost& g = panel_timeline->ghosts.at(i);
        Clip* c = NULL;
        if (g.clip != -1) c = sequence->clips.at(g.clip);

		MediaStream* ms = NULL;
		if (g.clip != -1 && c->media_type == MEDIA_TYPE_FOOTAGE) {
            ms = static_cast<Media*>(c->media)->get_stream_from_file_index(c->track < 0, c->media_stream);
		}

        // validate ghosts for trimming
		if (panel_timeline->creating) {
			// i feel like we might need something here but we haven't so far?
		} else if (panel_timeline->tool == TIMELINE_TOOL_SLIP) {
            if (c->media_type == MEDIA_TYPE_SEQUENCE
                    || (c->media_type == MEDIA_TYPE_FOOTAGE && !static_cast<Media*>(c->media)->get_stream_from_file_index(c->track < 0, c->media_stream)->infinite_length)) {
                // prevent slip moving a clip below 0 clip_in
                validator = g.old_clip_in - frame_diff;
                if (validator < 0) frame_diff += validator;

                // prevent slip moving clip beyond media length
                validator += g.ghost_length;
                if (validator > g.media_length) frame_diff += validator - g.media_length;
            }
		} else if (g.trimming) {
			if (g.trim_in) {
				// prevent clip/transition length from being less than 1 frame long
				validator = g.ghost_length - frame_diff;
				if (validator < 1) frame_diff -= (1 - validator);

				// prevent timeline in from going below 0
				validator = g.old_in + frame_diff;
				if (validator < 0) frame_diff -= validator;

				// prevent clip_in from going below 0
				if (c->media_type == MEDIA_TYPE_SEQUENCE
						|| (ms != NULL && !ms->infinite_length)) {
					validator = g.old_clip_in + frame_diff;
					if (validator < 0) frame_diff -= validator;
				}
            } else {
				// prevent clip length from being less than 1 frame long
				validator = g.ghost_length + frame_diff;
				if (validator < 1) frame_diff += (1 - validator);

				// prevent clip length exceeding media length
				if (c->media_type == MEDIA_TYPE_SEQUENCE
						|| (ms != NULL && !ms->infinite_length)) {
					validator = g.old_clip_in + g.ghost_length + frame_diff;
					if (validator > g.media_length) frame_diff -= validator - g.media_length;
				}
            }

            // ripple ops
            if (panel_timeline->tool == TIMELINE_TOOL_RIPPLE) {
                for (int j=0;j<post_clips.size();j++) {
                    Clip* post = post_clips.at(j);

                    // prevent any rippled clip from going below 0
                    if (panel_timeline->trim_in_point) {
                        validator = post->timeline_in - frame_diff;
                        if (validator < 0) frame_diff += validator;
                    }

                    // prevent any post-clips colliding with pre-clips
                    for (int k=0;k<pre_clips.size();k++) {
                        Clip* pre = pre_clips.at(k);
                        if (pre != post && pre->track == post->track) {
                            if (panel_timeline->trim_in_point) {
                                validator = post->timeline_in - frame_diff - pre->timeline_out;
                                if (validator < 0) frame_diff += validator;
                            } else {
                                validator = post->timeline_in + frame_diff - pre->timeline_out;
                                if (validator < 0) frame_diff -= validator;
                            }
                        }
                    }
                }
            }
        } else if (clips_are_movable) { // validate ghosts for moving
            // prevent clips from moving below 0 on the timeline
            validator = g.old_in + frame_diff;
            if (validator < 0) frame_diff -= validator;

			if (g.transition != NULL) {
				// prevent clip_in from going below 0
				if (c->media_type == MEDIA_TYPE_SEQUENCE
						|| (ms != NULL && !ms->infinite_length)) {
					validator = g.old_clip_in + frame_diff;
					if (validator < 0) frame_diff -= validator;
				}

				// prevent clip length exceeding media length
				if (c->media_type == MEDIA_TYPE_SEQUENCE
						|| (ms != NULL && !ms->infinite_length)) {
					validator = g.old_clip_in + g.ghost_length + frame_diff;
					if (validator > g.media_length) frame_diff -= validator - g.media_length;
				}
			}

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
        }
    }
    if (temp_frame_diff != frame_diff) panel_timeline->snapped = false;

    // apply changes to ghosts
    for (int i=0;i<panel_timeline->ghosts.size();i++) {
        Ghost& g = panel_timeline->ghosts[i];

        if (panel_timeline->tool == TIMELINE_TOOL_SLIP) {
            g.clip_in = g.old_clip_in - frame_diff;
        } else if (g.trimming) {
            if (g.trim_in) {
                g.in = g.old_in + frame_diff;
                g.clip_in = g.old_clip_in + frame_diff;
            } else {
                g.out = g.old_out + frame_diff;
            }
        } else if (clips_are_movable) {
            g.track = g.old_track;
            g.in = g.old_in + frame_diff;
            g.out = g.old_out + frame_diff;

            if (g.transition != NULL && g.transition == sequence->clips.at(g.clip)->opening_transition) {
				g.clip_in = g.old_clip_in + frame_diff;
			}

            if (panel_timeline->importing) {
                if ((panel_timeline->video_ghosts && mouse_track < 0)
                        || (panel_timeline->audio_ghosts && mouse_track >= 0)) {
                    int abs_track_diff = abs(track_diff);
                    if (g.old_track < 0) { // clip is video
                        g.track -= abs_track_diff;
                    } else { // clip is audio
                        g.track += abs_track_diff;
                    }
                }
			} else if (same_sign(g.old_track, panel_timeline->drag_track_start)) {
                g.track += track_diff;
            }
        }
    }

    // apply changes to selections
    if (panel_timeline->tool != TIMELINE_TOOL_SLIP && !panel_timeline->importing) {
		for (int i=0;i<sequence->selections.size();i++) {
			Selection& s = sequence->selections[i];
            if (panel_timeline->trim_target > -1) {
                if (panel_timeline->trim_in_point) {
                    s.in = s.old_in + frame_diff;
                } else {
                    s.out = s.old_out + frame_diff;
                }
            } else {
				for (int i=0;i<sequence->selections.size();i++) {
					Selection& s = sequence->selections[i];
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
        }
    }

    QToolTip::showText(mapToGlobal(mouse_pos), ((frame_diff < 0) ? "-" : "+") + frame_to_timecode(qAbs(frame_diff), config.timecode_view, sequence->frame_rate));
}

void TimelineWidget::mouseMoveEvent(QMouseEvent *event) {
    tooltip_timer.stop();
    if (sequence != NULL) {
        bool alt = (event->modifiers() & Qt::AltModifier);

		panel_timeline->cursor_frame = panel_timeline->getTimelineFrameFromScreenPoint(event->pos().x());
        panel_timeline->cursor_track = getTrackFromScreenPoint(event->pos().y());

		if (isLiveEditing()) {
			panel_timeline->snap_to_timeline(&panel_timeline->cursor_frame, !config.edit_tool_also_seeks || !panel_timeline->selecting, true, true);
        }
        if (panel_timeline->selecting) {
            int selection_count = 1 + qMax(panel_timeline->cursor_track, panel_timeline->drag_track_start) - qMin(panel_timeline->cursor_track, panel_timeline->drag_track_start) + panel_timeline->selection_offset;
			if (sequence->selections.size() != selection_count) {
				sequence->selections.resize(selection_count);
            }
            int minimum_selection_track = qMin(panel_timeline->cursor_track, panel_timeline->drag_track_start);
            for (int i=panel_timeline->selection_offset;i<selection_count;i++) {
				Selection& s = sequence->selections[i];
                s.track = minimum_selection_track + i - panel_timeline->selection_offset;
                long in = panel_timeline->drag_frame_start;
                long out = panel_timeline->cursor_frame;
                s.in = qMin(in, out);
                s.out = qMax(in, out);
            }

            // select linked clips too
            if (config.edit_tool_selects_links) {
                for (int j=0;j<sequence->clips.size();j++) {
                    Clip* c = sequence->clips.at(j);
					for (int k=0;k<sequence->selections.size();k++) {
						const Selection& s = sequence->selections.at(k);
                        if (!(c->timeline_in < s.in && c->timeline_out < s.in) &&
                                !(c->timeline_in > s.out && c->timeline_out > s.out) &&
                                c->track == s.track) {

                            QVector<int> linked_tracks = panel_timeline->get_tracks_of_linked_clips(j);
                            for (int k=0;k<linked_tracks.size();k++) {
                                bool found = false;
								for (int l=0;l<sequence->selections.size();l++) {
									const Selection& test_sel = sequence->selections.at(l);
                                    if (test_sel.track == linked_tracks.at(k) &&
                                            test_sel.in == s.in &&
                                            test_sel.out == s.out) {
                                        found = true;
                                        break;
                                    }
                                }
                                if (!found) {
                                    Selection link_sel = {s.in, s.out, linked_tracks.at(k)};
									sequence->selections.append(link_sel);
                                }
                            }

                            break;
                        }
                    }
                }
            }

            if (config.edit_tool_also_seeks) {
                panel_timeline->seek(qMin(panel_timeline->drag_frame_start, panel_timeline->cursor_frame));
            } else {
				panel_timeline->repaint_timeline(false);
            }
        } else if (panel_timeline->moving_init) {
            if (track_resizing) {
                int diff = track_resize_mouse_cache - event->pos().y();
                int new_height = track_resize_old_value;
                if (bottom_align) {
                    new_height += diff;
                } else {
                    new_height -= diff;
                }
                if (new_height < TRACK_MIN_HEIGHT) new_height = TRACK_MIN_HEIGHT;
                panel_timeline->calculate_track_height(track_target, new_height);
				update();
            } else if (panel_timeline->moving_proc) {
                QPoint pos = event->pos();
                update_ghosts(pos);
            } else {
                // set up movement
                // create ghosts
                for (int i=0;i<sequence->clips.size();i++) {
                    Clip* c = sequence->clips.at(i);
					if (c != NULL) {
                        Ghost g;
						g.transition = NULL;

						bool add = panel_timeline->is_clip_selected(c, true);

						// if a whole clip is not selected, maybe just a transition is
						if (!add && panel_timeline->tool == TIMELINE_TOOL_POINTER && (c->opening_transition != NULL || c->closing_transition != NULL)) {
							// check if any selections contain the whole clip or transition
							for (int j=0;j<sequence->selections.size();j++) {
								const Selection& s = sequence->selections.at(j);
								if (s.track == c->track) {
									if (c->opening_transition != NULL
											&& s.in == c->timeline_in
											&& s.out == c->timeline_in + c->opening_transition->length) {
										g.transition = c->opening_transition;
										add = true;
										break;
									} else if (c->closing_transition != NULL
											&& s.in == c->timeline_out - c->closing_transition->length
											&& s.out == c->timeline_out) {
										g.transition = c->closing_transition;
										add = true;
										break;
									}
								}
							}
						}

						if (add) {
							g.clip = i;
							g.trimming = (panel_timeline->trim_target > -1);
							g.trim_in = panel_timeline->trim_in_point;
							panel_timeline->ghosts.append(g);
						}
                    }
                }

                int size = panel_timeline->ghosts.size();
                if (panel_timeline->tool == TIMELINE_TOOL_ROLLING) {
                    for (int i=0;i<size;i++) {
                        Clip* ghost_clip = sequence->clips.at(panel_timeline->ghosts.at(i).clip);

                        // see if any ghosts are touching, in which case flip them
                        for (int k=0;k<size;k++) {
                            Clip* comp_clip = sequence->clips.at(panel_timeline->ghosts.at(k).clip);
                            if ((panel_timeline->trim_in_point && comp_clip->timeline_out == ghost_clip->timeline_in) ||
                                    (!panel_timeline->trim_in_point && comp_clip->timeline_in == ghost_clip->timeline_out)) {
                                panel_timeline->ghosts[k].trim_in = !panel_timeline->trim_in_point;
                            }
                        }
                    }

                    // then look for other clips we're touching
                    for (int i=0;i<size;i++) {
                        const Ghost& g = panel_timeline->ghosts.at(i);
                        Clip* ghost_clip = sequence->clips.at(g.clip);
                        for (int j=0;j<sequence->clips.size();j++) {
                            Clip* comp_clip = sequence->clips.at(j);
                            if (comp_clip->track == ghost_clip->track) {
                                if ((panel_timeline->trim_in_point && comp_clip->timeline_out == ghost_clip->timeline_in) ||
                                        (!panel_timeline->trim_in_point && comp_clip->timeline_in == ghost_clip->timeline_out)) {
                                    // see if this clip is already selected, and if so just switch the trim_in
                                    bool found = false;
                                    int duplicate_ghost_index;
                                    for (duplicate_ghost_index=0;duplicate_ghost_index<size;duplicate_ghost_index++) {
                                        if (panel_timeline->ghosts.at(duplicate_ghost_index).clip == j) {
                                            found = true;
                                            break;
                                        }
                                    }
                                    if (g.trim_in == panel_timeline->trim_in_point) {
                                        if (!found) {
                                            // add ghost for this clip with opposite trim_in
                                            Ghost gh;
											gh.transition = NULL;
                                            gh.clip = j;
                                            gh.trimming = (panel_timeline->trim_target > -1);
                                            gh.trim_in = !panel_timeline->trim_in_point;
                                            panel_timeline->ghosts.append(gh);
                                        }
                                    } else {
                                        if (found) {
                                            panel_timeline->ghosts.removeAt(duplicate_ghost_index);
                                            size--;
                                            if (duplicate_ghost_index < i) i--;
                                        }
                                    }
                                }
                            }
                        }
                    }
                } else if (panel_timeline->tool == TIMELINE_TOOL_SLIDE) {
                    for (int i=0;i<size;i++) {
                        const Ghost& g = panel_timeline->ghosts.at(i);
                        Clip* ghost_clip = sequence->clips.at(g.clip);
                        panel_timeline->ghosts[i].trimming = false;
                        for (int j=0;j<sequence->clips.size();j++) {
                            Clip* c = sequence->clips.at(j);
							if (c != NULL && c->track == ghost_clip->track) {
                                bool found = false;
                                for (int k=0;k<size;k++) {
                                    if (panel_timeline->ghosts.at(k).clip == j) {
                                        found = true;
                                        break;
                                    }
                                }
                                if (!found) {
                                    bool is_in = (c->timeline_in == ghost_clip->timeline_out);
                                    if (is_in || c->timeline_out == ghost_clip->timeline_in) {
                                        Ghost gh;
										gh.transition = NULL;
                                        gh.clip = j;
                                        gh.trimming = true;
                                        gh.trim_in = is_in;
                                        panel_timeline->ghosts.append(gh);
                                    }
                                }
                            }
                        }
                    }
                }

                init_ghosts();

                // ripple edit prep
                if (panel_timeline->tool == TIMELINE_TOOL_RIPPLE) {
                    QVector<Clip*> axis_ghosts;
                    for (int i=0;i<panel_timeline->ghosts.size();i++) {
                        Clip* c = sequence->clips.at(panel_timeline->ghosts.at(i).clip);
                        bool found = false;
                        for (int j=0;j<axis_ghosts.size();j++) {
                            Clip* cc = axis_ghosts.at(j);
                            if (c->track == cc->track &&
                                    ((panel_timeline->trim_in_point && c->timeline_in < cc->timeline_in) ||
                                     (!panel_timeline->trim_in_point && c->timeline_out > cc->timeline_out))) {
                                axis_ghosts[j] = c;
                                found = true;
                                break;
                            }
                        }
                        if (!found) {
                            axis_ghosts.append(c);
                        }
                    }

                    for (int i=0;i<sequence->clips.size();i++) {
                        Clip* c = sequence->clips.at(i);
                        if (c != NULL && !panel_timeline->is_clip_selected(c, true)) {
                            for (int j=0;j<axis_ghosts.size();j++) {
                                Clip* axis = axis_ghosts.at(j);
                                long comp_point = (panel_timeline->trim_in_point) ? axis->timeline_in : axis->timeline_out;
                                bool clip_is_pre = (c->timeline_in < comp_point);
                                QVector<Clip*>& clip_list = clip_is_pre ? pre_clips : post_clips;
                                bool found = false;
                                for (int k=0;k<clip_list.size();k++) {
                                    Clip* cached = clip_list.at(k);
                                    if (cached->track == c->track) {
                                        if (clip_is_pre == (c->timeline_in > cached->timeline_in)) {
                                            clip_list[k] = c;
                                        }
                                        found = true;
                                        break;
                                    }
                                }
                                if (!found) {
                                    clip_list.append(c);
                                }
                            }
                        }
                    }
                }

                panel_timeline->moving_proc = true;
            }
			panel_timeline->repaint_timeline(false);
        } else if (panel_timeline->splitting) {
            int track_start = qMin(panel_timeline->cursor_track, panel_timeline->drag_track_start);
            int track_end = qMax(panel_timeline->cursor_track, panel_timeline->drag_track_start);
            int track_size = 1 + track_end - track_start;
            panel_timeline->split_tracks.resize(track_size);
            for (int i=0;i<track_size;i++) {
                panel_timeline->split_tracks[i] = track_start + i;
            }

            if (!alt) {
                for (int i=0;i<track_size;i++) {
                    int clip_index = getClipIndexFromCoords(panel_timeline->drag_frame_start, panel_timeline->split_tracks[i]);
                    if (clip_index > -1) {
                        QVector<int> tracks = panel_timeline->get_tracks_of_linked_clips(clip_index);
                        for (int j=0;j<tracks.size();j++) {
                            // check if this track is already included
                            if (tracks.at(j) < track_start || tracks.at(j) > track_end) {
                                panel_timeline->split_tracks.append(tracks.at(j));
                            }
                        }
                    }
                }
            }
			panel_timeline->repaint_timeline(false);
        } else if (panel_timeline->rect_select_init) {
            if (panel_timeline->rect_select_proc) {
                panel_timeline->rect_select_w = event->pos().x() - panel_timeline->rect_select_x;
                panel_timeline->rect_select_h = event->pos().y() - panel_timeline->rect_select_y;
                if (bottom_align) panel_timeline->rect_select_h -= height();

				long frame_start = panel_timeline->getTimelineFrameFromScreenPoint(panel_timeline->rect_select_x);
				long frame_end = panel_timeline->getTimelineFrameFromScreenPoint(event->pos().x());
                long frame_min = qMin(frame_start, frame_end);
                long frame_max = qMax(frame_start, frame_end);

                int rsy = panel_timeline->rect_select_y;
                if (bottom_align) rsy += height();
                int track_start = getTrackFromScreenPoint(rsy);
                int track_end = getTrackFromScreenPoint(event->pos().y());
                int track_min = qMin(track_start, track_end);
                int track_max = qMax(track_start, track_end);

                QVector<Clip*> selected_clips;
                for (int i=0;i<sequence->clips.size();i++) {
                    Clip* clip = sequence->clips.at(i);
                    if (clip != NULL &&
                            clip->track >= track_min &&
                            clip->track <= track_max &&
                            !(clip->timeline_in < frame_min && clip->timeline_out < frame_min) &&
                            !(clip->timeline_in > frame_max && clip->timeline_out > frame_max)) {
                        QVector<Clip*> session_clips;
                        session_clips.append(clip);

                        if (!alt) {
                            for (int j=0;j<clip->linked.size();j++) {
                                session_clips.append(sequence->clips.at(clip->linked.at(j)));
                            }
                        }

                        for (int j=0;j<session_clips.size();j++) {
                            // see if clip has already been added - this can easily happen due to adding linked clips
                            bool found = false;
                            Clip* c = session_clips.at(j);
                            for (int k=0;k<selected_clips.size();k++) {
                                if (selected_clips.at(k) == c) {
                                    found = true;
                                    break;
                                }
                            }
                            if (!found) {
                                selected_clips.append(c);
                            }
                        }
                    }
                }

				sequence->selections.resize(selected_clips.size() + panel_timeline->selection_offset);
                for (int i=0;i<selected_clips.size();i++) {
					Selection& s = sequence->selections[i+panel_timeline->selection_offset];
                    Clip* clip = selected_clips.at(i);
                    s.old_in = s.in = clip->timeline_in;
                    s.old_out = s.out = clip->timeline_out;
                    s.old_track = s.track = clip->track;
                }

				panel_timeline->repaint_timeline(false);
            } else {
                panel_timeline->rect_select_x = event->pos().x();
                panel_timeline->rect_select_y = event->pos().y();
                if (bottom_align) panel_timeline->rect_select_y -= height();
                panel_timeline->rect_select_w = 0;
                panel_timeline->rect_select_h = 0;
                panel_timeline->rect_select_proc = true;
            }
		} else if (isLiveEditing()) {
			// redraw because we have a cursor
			panel_timeline->repaint_timeline(false);
        } else if (panel_timeline->tool == TIMELINE_TOOL_POINTER ||
                   panel_timeline->tool == TIMELINE_TOOL_RIPPLE ||
                   panel_timeline->tool == TIMELINE_TOOL_ROLLING) {
            track_resizing = false;

			QToolTip::hideText();

            QPoint pos = event->pos();

            int lim = 5;
            int mouse_track = getTrackFromScreenPoint(pos.y());
			long mouse_frame_lower = panel_timeline->getTimelineFrameFromScreenPoint(pos.x()-lim)-1;
			long mouse_frame_upper = panel_timeline->getTimelineFrameFromScreenPoint(pos.x()+lim)+1;
            bool found = false;
            bool cursor_contains_clip = false;
            int closeness = INT_MAX;
			panel_timeline->transition_select = TA_NO_TRANSITION;
            for (int i=0;i<sequence->clips.size();i++) {
                Clip* c = sequence->clips.at(i);
                if (c != NULL && c->track == mouse_track) {
                    if (panel_timeline->cursor_frame >= c->timeline_in &&
                            panel_timeline->cursor_frame <= c->timeline_out) {
                        cursor_contains_clip = true;

                        tooltip_timer.start();
                        tooltip_clip = i;

						if (c->opening_transition != NULL && panel_timeline->cursor_frame <= c->timeline_in + c->opening_transition->length) {
							panel_timeline->transition_select = TA_OPENING_TRANSITION;
						} else if (c->closing_transition != NULL && panel_timeline->cursor_frame >= c->timeline_out - c->closing_transition->length) {
							panel_timeline->transition_select = TA_CLOSING_TRANSITION;
						}
                    }
                    if (c->timeline_in > mouse_frame_lower && c->timeline_in < mouse_frame_upper) {
                        int nc = qAbs(c->timeline_in + 1 - panel_timeline->cursor_frame);
                        if (nc < closeness) {
                            panel_timeline->trim_target = i;
							panel_timeline->trim_in_point = true;
                            closeness = nc;
                            found = true;
                        }
                    }
                    if (c->timeline_out > mouse_frame_lower && c->timeline_out < mouse_frame_upper) {
                        int nc = qAbs(c->timeline_out - 1 - panel_timeline->cursor_frame);
                        if (nc < closeness) {
                            panel_timeline->trim_target = i;
							panel_timeline->trim_in_point = false;
                            closeness = nc;
                            found = true;
                        }
                    }
					if (panel_timeline->tool == TIMELINE_TOOL_POINTER) {
						if (c->opening_transition != NULL) {
							long transition_point = c->timeline_in + c->opening_transition->length;

							if (transition_point > mouse_frame_lower && transition_point < mouse_frame_upper) {
								int nc = qAbs(transition_point - 1 - panel_timeline->cursor_frame);
								if (nc < closeness) {
									panel_timeline->trim_target = i;
									panel_timeline->trim_in_point = false;
									panel_timeline->transition_select = TA_OPENING_TRANSITION;
									closeness = nc;
									found = true;
								}
							}
						}
						if (c->closing_transition != NULL) {
							long transition_point = c->timeline_out - c->closing_transition->length;
							if (transition_point > mouse_frame_lower && transition_point < mouse_frame_upper) {
								int nc = qAbs(transition_point + 1 - panel_timeline->cursor_frame);
								if (nc < closeness) {
									panel_timeline->trim_target = i;
									panel_timeline->trim_in_point = true;
									panel_timeline->transition_select = TA_CLOSING_TRANSITION;
									closeness = nc;
									found = true;
								}
							}
						}
					}
                }
            }
            /*if (cursor_contains_clip) {
				QToolTip::showText(mapToGlobal(event->pos()), "HOVER OVER CLIP");
            }*/
            if (found) {
                setCursor(Qt::SizeHorCursor);
            } else {
                panel_timeline->trim_target = -1;

                // look for track heights
                int track_y = 0;
                for (int i=0;i<panel_timeline->get_track_height_size(bottom_align);i++) {
                    int track = (bottom_align) ? -1-i : i;
                    int track_height = panel_timeline->calculate_track_height(track, -1);
                    track_y += track_height;
                    int y_test_value = (bottom_align) ? rect().bottom() - track_y : track_y;
                    int test_range = 5;
                    if (pos.y() > y_test_value-test_range && pos.y() < y_test_value+test_range) {
                        // if track lines are hidden, only resize track if a clip is already there
                        if (config.show_track_lines || cursor_contains_clip) {
                            found = true;
                            track_resizing = true;
                            track_target = track;
                            track_resize_old_value = track_height;
                        }
                        break;
                    }
                }

                if (found) {
                    setCursor(Qt::SizeVerCursor);
                } else {
                    unsetCursor();
                }
			}
        } else if (panel_timeline->tool == TIMELINE_TOOL_SLIP) {
            if (getClipIndexFromCoords(panel_timeline->cursor_frame, panel_timeline->cursor_track) > -1) {
                setCursor(Qt::SizeHorCursor);
            } else {
                unsetCursor();
            }
        }
    }
}

void TimelineWidget::leaveEvent(QEvent *event) {
    tooltip_timer.stop();
}

int color_brightness(int r, int g, int b) {
	return (0.2126*r + 0.7152*g + 0.0722*b);
}

void TimelineWidget::paintEvent(QPaintEvent*) {
	// Draw clips
	if (sequence != NULL) {
		QPainter p(this);

		// get widget width and height
		int video_track_limit = 0;
		int audio_track_limit = 0;
		for (int i=0;i<sequence->clips.size();i++) {
			Clip* clip = sequence->clips.at(i);
			if (clip != NULL) {
				video_track_limit = qMin(video_track_limit, clip->track);
				audio_track_limit = qMax(audio_track_limit, clip->track);
			}
		}

		int panel_height = 100;
		if (bottom_align) {
			for (int i=-1;i>=video_track_limit;i--) {
				panel_height += panel_timeline->calculate_track_height(i, -1);
			}
		} else {
			for (int i=0;i<=audio_track_limit;i++) {
				panel_height += panel_timeline->calculate_track_height(i, -1);
			}
		}

		QColor transition_color(255, 0, 0, 16);
		for (int i=0;i<sequence->clips.size();i++) {
			Clip* clip = sequence->clips.at(i);
			if (clip != NULL && is_track_visible(clip->track)) {
				QRect clip_rect(panel_timeline->getTimelineScreenPointFromFrame(clip->timeline_in), getScreenPointFromTrack(clip->track), clip->getLength() * panel_timeline->zoom, panel_timeline->calculate_track_height(clip->track, -1));
				QRect text_rect(clip_rect.left() + CLIP_TEXT_PADDING, clip_rect.top() + CLIP_TEXT_PADDING, clip_rect.width() - CLIP_TEXT_PADDING - 1, clip_rect.height() - CLIP_TEXT_PADDING - 1);
				if (clip_rect.left() < width() && clip_rect.right() >= 0) {
					QRect actual_clip_rect = clip_rect;
					if (actual_clip_rect.x() < 0) actual_clip_rect.setX(0);
					if (actual_clip_rect.right() >= width()) actual_clip_rect.setRight(width());
					p.fillRect(actual_clip_rect, QColor(clip->color_r, clip->color_g, clip->color_b));

					int thumb_x = clip_rect.x() + 1;

					if (clip->media_type == MEDIA_TYPE_FOOTAGE) {
						bool draw_checkerboard = false;
						QRect checkerboard_rect(clip_rect);
						Media* m = static_cast<Media*>(clip->media);
						MediaStream* ms = m->get_stream_from_file_index(clip->track < 0, clip->media_stream);
						if (ms == NULL) {
							draw_checkerboard = true;
						} else if (ms->preview_done) {
							// draw thumbnail/waveform
							long media_length = clip->getMaximumLength();

							if (clip->track < 0) {
								// draw thumbnail
								if (thumb_x < width()) {
									int space_for_thumb = clip_rect.width();
									if (clip->opening_transition != NULL) {
										int ot_width = getScreenPointFromFrame(panel_timeline->zoom, clip->opening_transition->length);
										thumb_x += ot_width;
										space_for_thumb -= ot_width;
									}
									if (clip->closing_transition != NULL) {
										space_for_thumb -= getScreenPointFromFrame(panel_timeline->zoom, clip->closing_transition->length);
									}
									int thumb_y = p.fontMetrics().height()+CLIP_TEXT_PADDING+CLIP_TEXT_PADDING;
									int thumb_height = clip_rect.height()-thumb_y;
									int thumb_width = (thumb_height*((double)ms->video_preview.width()/(double)ms->video_preview.height()));
									if (thumb_x + thumb_width >= 0
											&& thumb_height > thumb_y
											&& text_rect.width() + CLIP_TEXT_PADDING > thumb_width
											&& space_for_thumb >= thumb_width) { // at small clip heights, don't even draw it
										QRect thumb_rect(thumb_x, clip_rect.y()+thumb_y, thumb_width, thumb_height);
										p.drawImage(thumb_rect, ms->video_preview);
									}
								}
							} else if (clip_rect.height() > TRACK_MIN_HEIGHT) {
								int waveform_start = -qMin(clip_rect.x(), 0);
								int waveform_limit = qMin(clip_rect.width(), getScreenPointFromFrame(panel_timeline->zoom, media_length - clip->clip_in));

								if ((clip_rect.x() + waveform_limit) > width()) {
									waveform_limit -= (clip_rect.x() + waveform_limit - width());
								} else if (waveform_limit < clip_rect.width()) {
									draw_checkerboard = true;
									if (waveform_limit > 0) checkerboard_rect.setLeft(checkerboard_rect.left() + waveform_limit);
								}

								// draw waveform
								p.setPen(QColor(80, 80, 80));

								int divider = ms->audio_channels*2;
								int channel_height = clip_rect.height()/ms->audio_channels;

								for (int i=waveform_start;i<waveform_limit;i++) {
									int waveform_index = qFloor((((clip->clip_in + ((double) i/panel_timeline->zoom))/media_length) * ms->audio_preview.size())/divider)*divider;

									if (clip->reverse) {
										waveform_index = ms->audio_preview.size() - waveform_index - (ms->audio_channels * 2);
									}

									int rectified_height = 0;

									for (int j=0;j<ms->audio_channels;j++) {
										int mid = clip_rect.top()+channel_height*j+(channel_height/2);
										int offset = waveform_index+(j*2);

										if ((offset + 1) < ms->audio_preview.size()) {
											qint8 min = (double)ms->audio_preview.at(offset) / 128.0 * (channel_height/2);
											qint8 max = (double)ms->audio_preview.at(offset+1) / 128.0 * (channel_height/2);

											if (config.rectified_waveforms)  {
												rectified_height += (max - min);
											} else {
												p.drawLine(clip_rect.left()+i, mid+min, clip_rect.left()+i, mid+max);
											}
										} else {
											qDebug() << "[WARNING] Tried to reach" << offset + 1 << ", limit:" << ms->audio_preview.size();
										}
									}

									if (config.rectified_waveforms) {
										p.drawLine(clip_rect.left()+i, clip_rect.height(), clip_rect.left()+i, clip_rect.height() - rectified_height);
									}
								}
							}
						}
						if (draw_checkerboard) {
							checkerboard_rect.setLeft(qMax(checkerboard_rect.left(), 0));
							checkerboard_rect.setRight(qMin(checkerboard_rect.right(), width()));

							if (checkerboard_rect.left() < width() && checkerboard_rect.right() >= 0) {
								// draw "error lines" if media stream is missing
								p.setPen(QPen(QColor(64, 64, 64), 2));
								int limit = checkerboard_rect.width();
								int clip_height = checkerboard_rect.height();
								for (int j=-clip_height;j<limit;j+=15) {
									int lines_start_x = checkerboard_rect.left()+j;
									int lines_start_y = checkerboard_rect.bottom();
									int lines_end_x = lines_start_x + clip_height;
									int lines_end_y = checkerboard_rect.top();
									if (lines_start_x < checkerboard_rect.left()) {
										lines_start_y -= (checkerboard_rect.left() - lines_start_x);
										lines_start_x = checkerboard_rect.left();
									}
									if (lines_end_x > checkerboard_rect.right()) {
										lines_end_y -= (checkerboard_rect.right() - lines_end_x);
										lines_end_x = checkerboard_rect.right();
									}
									p.drawLine(lines_start_x, lines_start_y, lines_end_x, lines_end_y);
								}
							}
						}
					}

					// draw clip transitions
					for (int i=0;i<2;i++) {
						Transition* t = (i == 0) ? clip->opening_transition : clip->closing_transition;
						if (t != NULL) {
							int transition_width = panel_timeline->getTimelineScreenPointFromFrame(t->length);
							int transition_height = clip_rect.height();
							int tr_y = clip_rect.y();
							int tr_x = 0;
							if (i == 0) {
								tr_x = clip_rect.x();
								text_rect.setX(text_rect.x()+transition_width);
							} else {
								tr_x = clip_rect.right()-transition_width;
								text_rect.setWidth(text_rect.width()-transition_width);
							}
							QRect transition_rect = QRect(tr_x, tr_y, transition_width, transition_height);
							p.fillRect(transition_rect, transition_color);
							QRect transition_text_rect(transition_rect.x() + CLIP_TEXT_PADDING, transition_rect.y() + CLIP_TEXT_PADDING, transition_rect.width() - CLIP_TEXT_PADDING, transition_rect.height() - CLIP_TEXT_PADDING);
							if (transition_text_rect.width() > MAX_TEXT_WIDTH) {
								p.setPen(QColor(0, 0, 0, 96));
								if (i == 0) {
									p.drawLine(transition_rect.bottomLeft(), transition_rect.topRight());
								} else {
									p.drawLine(transition_rect.topLeft(), transition_rect.bottomRight());
								}

								p.setPen(Qt::white);
								p.drawText(transition_text_rect, 0, t->name, &transition_text_rect);
							}
							p.setPen(Qt::black);
							p.drawRect(transition_rect);
						}
					}

					// top left bevel
					p.setPen(Qt::white);
					if (clip_rect.x() >= 0) p.drawLine(clip_rect.bottomLeft(), clip_rect.topLeft());
					p.drawLine(QPoint(qMax(0, clip_rect.left()), qMax(0, clip_rect.top())), QPoint(qMin(width(), clip_rect.right()), qMax(0, clip_rect.top())));

					// draw text
					if (text_rect.width() > MAX_TEXT_WIDTH && text_rect.right() > 0 && text_rect.left() < width()) {
						if (color_brightness(clip->color_r, clip->color_g, clip->color_b) > 160) {
							// set to black if color is bright
							p.setPen(Qt::black);
						}
						if (clip->linked.size() > 0) {
							int underline_y = CLIP_TEXT_PADDING + p.fontMetrics().height() + clip_rect.top();
							int underline_width = qMin(text_rect.width() - 1, p.fontMetrics().width(clip->name));
							p.drawLine(text_rect.x(), underline_y, text_rect.x() + underline_width, underline_y);
						}
						QString name = clip->name;
						if (clip->speed != 1.0 || clip->reverse) {
							name += " (";
							if (clip->reverse) name += "-";
							name += QString::number(clip->speed*100) + "%)";
						}
						p.drawText(text_rect, 0, name, &text_rect);
					}

					// bottom right gray
					p.setPen(QColor(0, 0, 0, 128));
					if (clip_rect.right() < width()) p.drawLine(clip_rect.bottomRight(), clip_rect.topRight());
					p.drawLine(QPoint(qMax(0, clip_rect.left()), qMin(height(), clip_rect.bottom())), QPoint(qMin(width(), clip_rect.right()), qMin(height(), clip_rect.bottom())));
				}
			}
		}

		// Draw track lines
		if (config.show_track_lines) {
			p.setPen(QColor(0, 0, 0, 96));
			audio_track_limit++;
			if (video_track_limit == 0) video_track_limit--;

			if (bottom_align) {
				// only draw lines for video tracks
				for (int i=video_track_limit;i<0;i++) {
					int line_y = getScreenPointFromTrack(i) - 1;
					p.drawLine(0, line_y, rect().width(), line_y);
				}
			} else {
				// only draw lines for audio tracks
				for (int i=0;i<audio_track_limit;i++) {
					int line_y = getScreenPointFromTrack(i) + panel_timeline->calculate_track_height(i, -1);
					p.drawLine(0, line_y, rect().width(), line_y);
				}
			}
		}

		// Draw selections
		for (int i=0;i<sequence->selections.size();i++) {
			const Selection& s = sequence->selections.at(i);
			if (is_track_visible(s.track)) {
				int selection_y = getScreenPointFromTrack(s.track);
				int selection_x = panel_timeline->getTimelineScreenPointFromFrame(s.in);
				p.fillRect(selection_x, selection_y, panel_timeline->getTimelineScreenPointFromFrame(s.out) - selection_x, panel_timeline->calculate_track_height(s.track, -1), QColor(0, 0, 0, 64));
			}
		}

        // draw rectangle select
        if (panel_timeline->rect_select_proc) {
            int rsy = panel_timeline->rect_select_y;
            int rsh = panel_timeline->rect_select_h;
            if (bottom_align) {
                rsy += height();
            }
			QRect rect_select(panel_timeline->rect_select_x, rsy, panel_timeline->rect_select_w, rsh);
			draw_selection_rectangle(p, rect_select);
        }

		// Draw ghosts
		for (int i=0;i<panel_timeline->ghosts.size();i++) {
			const Ghost& g = panel_timeline->ghosts.at(i);
			if (is_track_visible(g.track)) {
				int ghost_x = panel_timeline->getTimelineScreenPointFromFrame(g.in);
				int ghost_y = getScreenPointFromTrack(g.track);
				int ghost_width = panel_timeline->getTimelineScreenPointFromFrame(g.out) - ghost_x - 1;
                int ghost_height = panel_timeline->calculate_track_height(g.track, -1) - 1;
				p.setPen(QColor(255, 255, 0));
				for (int j=0;j<GHOST_THICKNESS;j++) {
					p.drawRect(ghost_x+j, ghost_y+j, ghost_width-j-j, ghost_height-j-j);
				}
			}
		}

        // Draw splitting cursor
        if (panel_timeline->splitting) {
            for (int i=0;i<panel_timeline->split_tracks.size();i++) {
                if (is_track_visible(panel_timeline->split_tracks.at(i))) {
					int cursor_x = panel_timeline->getTimelineScreenPointFromFrame(panel_timeline->drag_frame_start);
                    int cursor_y = getScreenPointFromTrack(panel_timeline->split_tracks.at(i));

					p.setPen(QColor(64, 64, 64));
					p.drawLine(cursor_x, cursor_y, cursor_x, cursor_y + panel_timeline->calculate_track_height(panel_timeline->split_tracks.at(i), -1));
                }
            }
        }

        // Draw playhead
        p.setPen(Qt::red);
		int playhead_x = panel_timeline->getTimelineScreenPointFromFrame(sequence->playhead);
        p.drawLine(playhead_x, rect().top(), playhead_x, rect().bottom());

        // draw border
        p.setPen(QColor(0, 0, 0, 64));
        int edge_y = (bottom_align) ? rect().height()-1 : 0;
        p.drawLine(0, edge_y, rect().width(), edge_y);

        // draw snap point
        if (panel_timeline->snapped) {
            p.setPen(Qt::white);
			int snap_x = panel_timeline->getTimelineScreenPointFromFrame(panel_timeline->snap_point);
            p.drawLine(snap_x, 0, snap_x, height());
        }

		// Draw edit cursor
		if (isLiveEditing()) {
            if (is_track_visible(panel_timeline->cursor_track)) {
				int cursor_x = panel_timeline->getTimelineScreenPointFromFrame(panel_timeline->cursor_frame);
				int cursor_y = getScreenPointFromTrack(panel_timeline->cursor_track);

				p.setPen(Qt::gray);
                p.drawLine(cursor_x, cursor_y, cursor_x, cursor_y + panel_timeline->calculate_track_height(panel_timeline->cursor_track, -1));
            }
		}
	}
}

bool TimelineWidget::is_track_visible(int track) {
    return (bottom_align == (track < 0));
}

// **************************************
// screen point <-> frame/track functions
// **************************************

int TimelineWidget::getTrackFromScreenPoint(int y) {
    if (bottom_align) {
        y = -(y - rect().height());
    }
    y--;
    int height_measure = 0;
    int counter = ((!bottom_align && y > 0) || (bottom_align && y < 0)) ? 0 : -1;
    int track_height = panel_timeline->calculate_track_height(counter, -1);
    while (qAbs(y) > height_measure+track_height) {
        if (config.show_track_lines && counter != -1) y--;
        height_measure += track_height;
        if ((!bottom_align && y > 0) || (bottom_align && y < 0)) {
            counter++;
        } else {
            counter--;
        }
        track_height = panel_timeline->calculate_track_height(counter, -1);
    }
    return counter;
}

int TimelineWidget::getScreenPointFromTrack(int track) {
    int y = 0;
    int counter = 0;
    while (counter != track) {
        if (bottom_align) counter--;
        y += panel_timeline->calculate_track_height(counter, -1);
        if (!bottom_align) counter++;
        if (config.show_track_lines && counter != -1) y++;
    }
    y++;
    return (bottom_align) ? rect().height() - y : y;
}

int TimelineWidget::getClipIndexFromCoords(long frame, int track) {
    for (int i=0;i<sequence->clips.size();i++) {
        Clip* c = sequence->clips.at(i);
        if (c != NULL && c->track == track && frame >= c->timeline_in && frame < c->timeline_out) {
            return i;
		}
	}
	return -1;
}
