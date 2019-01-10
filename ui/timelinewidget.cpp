#include "timelinewidget.h"

#include "playback/audio.h"
#include "panels/panels.h"
#include "io/config.h"
#include "project/sequence.h"
#include "project/transition.h"
#include "project/clip.h"
#include "panels/project.h"
#include "panels/timeline.h"
#include "project/footage.h"
#include "ui/sourcetable.h"
#include "ui/sourceiconview.h"
#include "panels/effectcontrols.h"
#include "panels/viewer.h"
#include "project/undo.h"
#include "mainwindow.h"
#include "ui/viewerwidget.h"
#include "dialogs/stabilizerdialog.h"
#include "project/media.h"
#include "ui/resizablescrollbar.h"
#include "dialogs/newsequencedialog.h"
#include "mainwindow.h"
#include "ui/rectangleselect.h"
#include "debug.h"

#include "project/effect.h"

#include "effects/internal/solideffect.h"
#include "effects/internal/texteffect.h"

#include <QPainter>
#include <QColor>
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
#include <QInputDialog>
#include <QStatusBar>

#define MAX_TEXT_WIDTH 20
#define TRANSITION_BETWEEN_RANGE 40

TimelineWidget::TimelineWidget(QWidget *parent) : QWidget(parent) {
	selection_command = NULL;
	self_created_sequence = NULL;
	scroll = 0;

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
	QVector<Selection> sels;

	Selection s;
	s.in = rc_ripple_min;
	s.out = rc_ripple_max;
	s.track = panel_timeline->cursor_track;

	sels.append(s);

	panel_timeline->delete_selection(sels, true);
}

void TimelineWidget::show_context_menu(const QPoint& pos) {
	if (sequence != NULL) {
		// hack because sometimes right clicking doesn't trigger mouse release event
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

		// collect all the selected clips
		QVector<Clip*> selected_clips;
		for (int i=0;i<sequence->clips.size();i++) {
			Clip* c = sequence->clips.at(i);
			if (c != NULL && panel_timeline->is_clip_selected(c, true)) {
				selected_clips.append(c);
			}
		}

		if (selected_clips.isEmpty()) {
			// no clips are selected
			panel_timeline->cursor_frame = panel_timeline->getTimelineFrameFromScreenPoint(pos.x());
			panel_timeline->cursor_track = getTrackFromScreenPoint(pos.y());

			bool can_ripple_delete = true;
			bool at_end_of_sequence = true;
			rc_ripple_min = 0;
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
				QAction* ripple_delete_action = menu.addAction("R&ipple Delete");
				connect(ripple_delete_action, SIGNAL(triggered(bool)), this, SLOT(right_click_ripple()));
			}

			QAction* seq_settings = menu.addAction("Sequence Settings");
			connect(seq_settings, SIGNAL(triggered(bool)), this, SLOT(open_sequence_properties()));
		} else {
			// clips are selected
			QAction* cutAction = menu.addAction("C&ut");
			connect(cutAction, SIGNAL(triggered(bool)), mainWindow, SLOT(cut()));
			QAction* copyAction = menu.addAction("Cop&y");
			connect(copyAction, SIGNAL(triggered(bool)), mainWindow, SLOT(copy()));
			QAction* pasteAction = menu.addAction("&Paste");
			connect(pasteAction, SIGNAL(triggered(bool)), mainWindow, SLOT(paste()));
			menu.addSeparator();
			QAction* speedAction = menu.addAction("&Speed/Duration");
			connect(speedAction, SIGNAL(triggered(bool)), mainWindow, SLOT(open_speed_dialog()));
			QAction* autoscaleAction = menu.addAction("Auto-s&cale");
			autoscaleAction->setCheckable(true);
			connect(autoscaleAction, SIGNAL(triggered(bool)), this, SLOT(toggle_autoscale()));

			QAction* nestAction = menu.addAction("&Nest");
			connect(nestAction, SIGNAL(triggered(bool)), mainWindow, SLOT(nest()));

			// stabilizer option
			/*int video_clip_count = 0;
			bool all_video_is_footage = true;
			for (int i=0;i<selected_clips.size();i++) {
				if (selected_clips.at(i)->track < 0) {
					video_clip_count++;
					if (selected_clips.at(i)->media == NULL
							|| selected_clips.at(i)->media->get_type() != MEDIA_TYPE_FOOTAGE) {
						all_video_is_footage = false;
					}
				}
			}
			if (video_clip_count == 1 && all_video_is_footage) {
				QAction* stabilizerAction = menu.addAction("S&tabilizer");
				connect(stabilizerAction, SIGNAL(triggered(bool)), this, SLOT(show_stabilizer_diag()));
			}*/

			// set autoscale arbitrarily to the first selected clip
			autoscaleAction->setChecked(selected_clips.at(0)->autoscale);

			// check if all selected clips have the same media for a "Reveal In Project"
			bool same_media = true;
			rc_reveal_media = selected_clips.at(0)->media;
			for (int i=1;i<selected_clips.size();i++) {
				if (selected_clips.at(i)->media != rc_reveal_media) {
					same_media = false;
					break;
				}
			}

			if (same_media) {
				QAction* revealInProjectAction = menu.addAction("&Reveal in Project");
				connect(revealInProjectAction, SIGNAL(triggered(bool)), this, SLOT(reveal_media()));
			}

			QAction* rename = menu.addAction("R&ename");
			connect(rename, SIGNAL(triggered(bool)), this, SLOT(rename_clip()));
		}

		menu.exec(mapToGlobal(pos));
	}
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
		if (tooltip_clip < sequence->clips.size()) {
			Clip* c = sequence->clips.at(tooltip_clip);
			if (c != NULL) {
				QToolTip::showText(QCursor::pos(),
							c->name
								   + "\nStart: " + frame_to_timecode(c->timeline_in, config.timecode_view, sequence->frame_rate)
								   + "\nEnd: " + frame_to_timecode(c->timeline_out, config.timecode_view, sequence->frame_rate)
								   + "\nDuration: " + frame_to_timecode(c->getLength(), config.timecode_view, sequence->frame_rate));
			}
		}
	}
	tooltip_timer.stop();
}

void TimelineWidget::rename_clip() {
	QVector<Clip*> selected_clips;
	for (int i=0;i<sequence->clips.size();i++) {
		Clip* c = sequence->clips.at(i);
		if (c != NULL && panel_timeline->is_clip_selected(c, true)) {
			selected_clips.append(c);
		}
	}
	if (selected_clips.size() > 0) {
		QString s = QInputDialog::getText(this,
										  (selected_clips.size() == 1) ? "Rename '" + selected_clips.at(0)->name + "'" : "Rename multiple clips",
										  "Enter a new name for this clip:",
										  QLineEdit::Normal,
										  selected_clips.at(0)->name
									);
		if (!s.isEmpty()) {
			RenameClipCommand* rcc = new RenameClipCommand();
			rcc->new_name = s;
			rcc->clips = selected_clips;
			undo_stack.push(rcc);
			update_ui(true);
		}
	}
}

void TimelineWidget::show_stabilizer_diag() {
	StabilizerDialog sd;
	sd.exec();
}

void TimelineWidget::open_sequence_properties() {
	QList<Media*> sequence_items;
	QList<Media*> all_top_level_items;
	for (int i=0;i<project_model.childCount();i++) {
		all_top_level_items.append(project_model.child(i));
	}
	panel_project->get_all_media_from_table(all_top_level_items, sequence_items, MEDIA_TYPE_SEQUENCE); // find all sequences in project
	for (int i=0;i<sequence_items.size();i++) {
		if (sequence_items.at(i)->to_sequence() == sequence) {
			NewSequenceDialog nsd(this, sequence_items.at(i));
			nsd.exec();
			return;
		}
	}
	QMessageBox::critical(this, "Error", "Couldn't locate media wrapper for sequence.");
}

bool same_sign(int a, int b) {
	return (a < 0) == (b < 0);
}

void TimelineWidget::dragEnterEvent(QDragEnterEvent *event) {
	bool import_init = false;

	QVector<Media*> media_list;
	panel_timeline->importing_files = false;

	if (event->source() == panel_project->tree_view || event->source() == panel_project->icon_view) {
		QModelIndexList items = panel_project->get_current_selected();
		media_list.resize(items.size());
		for (int i=0;i<items.size();i++) {
			media_list[i] = panel_project->item_to_media(items.at(i));
		}
		import_init = true;
	}

	if (event->source() == panel_footage_viewer->viewer_widget) {
		Sequence* proposed_seq = panel_footage_viewer->seq;
		if (proposed_seq != sequence) { // don't allow nesting the same sequence
			media_list.append(panel_footage_viewer->media);
			import_init = true;
		}
	}

	if (config.enable_drag_files_to_timeline && event->mimeData()->hasUrls()) {
		QList<QUrl> urls = event->mimeData()->urls();
		if (!urls.isEmpty()) {
			QStringList file_list;

			for (int i=0;i<urls.size();i++) {
				file_list.append(urls.at(i).toLocalFile());
			}

			panel_project->process_file_list(file_list);

			for (int i=0;i<panel_project->last_imported_media.size();i++) {
				// waits for media to have a duration
				// TODO would be much nicer if this was multithreaded
				Footage* f = panel_project->last_imported_media.at(i)->to_footage();
				f->ready_lock.lock();
				f->ready_lock.unlock();

				if (f->ready) {
					media_list.append(panel_project->last_imported_media.at(i));
				}
			}

			if (media_list.isEmpty()) {
				undo_stack.undo();
			} else {
				import_init = true;
				panel_timeline->importing_files = true;
			}
		}
	}

	if (import_init) {
		event->acceptProposedAction();

		long entry_point;
		Sequence* seq = sequence;

		if (seq == NULL) {
			// if no sequence, we're going to create a new one using the clips as a reference
			entry_point = 0;

			self_created_sequence = create_sequence_from_media(media_list);
			seq = self_created_sequence;
		} else {
			entry_point = panel_timeline->getTimelineFrameFromScreenPoint(event->pos().x());
			panel_timeline->drag_frame_start = entry_point + getFrameFromScreenPoint(panel_timeline->zoom, 50);
			panel_timeline->drag_track_start = (bottom_align) ? -1 : 0;
		}

		panel_timeline->create_ghosts_from_media(seq, entry_point, media_list);

		panel_timeline->importing = true;
	}
}

void TimelineWidget::dragMoveEvent(QDragMoveEvent *event) {
	if (panel_timeline->importing) {
		event->acceptProposedAction();

		if (sequence != NULL) {
			QPoint pos = event->pos();
			update_ghosts(pos, event->keyboardModifiers() & Qt::ShiftModifier);
			panel_timeline->move_insert = ((event->keyboardModifiers() & Qt::ControlModifier) && (panel_timeline->tool == TIMELINE_TOOL_POINTER || panel_timeline->importing));
			update_ui(false);
		}
	}
}

void TimelineWidget::wheelEvent(QWheelEvent *event) {
	bool alt = (event->modifiers() & Qt::AltModifier);
	int scroll_amount = alt ? (event->angleDelta().x()) : (event->angleDelta().y());
	if (scroll_amount != 0) {
		bool shift = (event->modifiers() & Qt::ShiftModifier);
		bool in = (scroll_amount > 0);
		if (config.scroll_zooms != shift) {
			panel_timeline->set_zoom(in);
		} else {
			QScrollBar* bar = alt ? scrollBar : panel_timeline->horizontalScrollBar;

			int step = bar->singleStep();
			if (in) step = -step;
			bar->setValue(bar->value() + step);
		}
	}
}

void TimelineWidget::dragLeaveEvent(QDragLeaveEvent* event) {
	event->accept();
	if (panel_timeline->importing) {
		if (panel_timeline->importing_files) {
			undo_stack.undo();
		}
		panel_timeline->importing_files = false;
		panel_timeline->ghosts.clear();
		panel_timeline->importing = false;
		update_ui(false);
	}
	if (self_created_sequence != NULL) {
		delete self_created_sequence;
		self_created_sequence = NULL;
	}
}

void delete_area_under_ghosts(ComboAction* ca) {
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

void insert_clips(ComboAction* ca) {
	bool ripple_old_point = true;

	long earliest_old_point = LONG_MAX;
	long latest_old_point = LONG_MIN;

	long earliest_new_point = LONG_MAX;
	long latest_new_point = LONG_MIN;

	QVector<int> ignore_clips;
	for (int i=0;i<panel_timeline->ghosts.size();i++) {
		const Ghost& g = panel_timeline->ghosts.at(i);

		earliest_old_point = qMin(earliest_old_point, g.old_in);
		latest_old_point = qMax(latest_old_point, g.old_out);
		earliest_new_point = qMin(earliest_new_point, g.in);
		latest_new_point = qMax(latest_new_point, g.out);

		if (g.clip >= 0) {
			ignore_clips.append(g.clip);
		} else {
			// don't try to close old gap if importing
			ripple_old_point = false;
		}
	}

	panel_timeline->split_cache.clear();

	for (int i=0;i<sequence->clips.size();i++) {
		Clip* c = sequence->clips.at(i);
		if (c != NULL) {
			// don't split any clips that are moving
			bool found = false;
			for (int j=0;j<panel_timeline->ghosts.size();j++) {
				if (panel_timeline->ghosts.at(j).clip == i) {
					found = true;
					break;
				}
			}
			if (!found) {
				if (c->timeline_in < earliest_new_point && c->timeline_out > earliest_new_point) {
					panel_timeline->split_clip_and_relink(ca, i, earliest_new_point, true);
				}

				// determine if we should close the gap the old clips left behind
				if (ripple_old_point
						&& !((c->timeline_in < earliest_old_point && c->timeline_out <= earliest_old_point) || (c->timeline_in >= latest_old_point && c->timeline_out > latest_old_point))
						&& !ignore_clips.contains(i)) {
					ripple_old_point = false;
				}
			}
		}
	}

	long ripple_length = (latest_new_point - earliest_new_point);

	ripple_clips(ca, sequence, earliest_new_point, ripple_length, ignore_clips);

	if (ripple_old_point) {
		// works for moving later clips earlier but not earlier to later
		long second_ripple_length = (earliest_old_point - latest_old_point);

		ripple_clips(ca, sequence, latest_old_point, second_ripple_length, ignore_clips);

		if (earliest_old_point < earliest_new_point) {
			for (int i=0;i<panel_timeline->ghosts.size();i++) {
				Ghost& g = panel_timeline->ghosts[i];
				g.in += second_ripple_length;
				g.out += second_ripple_length;
			}
			for (int i=0;i<sequence->selections.size();i++) {
				Selection& s = sequence->selections[i];
				s.in += second_ripple_length;
				s.out += second_ripple_length;
			}
		}
	}
}

void TimelineWidget::dropEvent(QDropEvent* event) {
	if (panel_timeline->importing && panel_timeline->ghosts.size() > 0) {
		event->acceptProposedAction();

		ComboAction* ca = new ComboAction();

		Sequence* s = sequence;

		// if we're dropping into nothing, create a new sequences based on the clip being dragged
		if (s == NULL) {
			s = self_created_sequence;
			panel_project->new_sequence(ca, self_created_sequence, true, NULL);
			self_created_sequence = NULL;
		} else if (event->keyboardModifiers() & Qt::ControlModifier) {
			insert_clips(ca);
		} else {
			delete_area_under_ghosts(ca);
		}

		panel_timeline->add_clips_from_ghosts(ca, s);

		undo_stack.push(ca);

		setFocus();

		update_ui(true);
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
			update_ui(false);
		}
	}
}

bool isLiveEditing() {
	return (panel_timeline->tool == TIMELINE_TOOL_EDIT || panel_timeline->tool == TIMELINE_TOOL_RAZOR || panel_timeline->creating);
}

void TimelineWidget::mousePressEvent(QMouseEvent *event) {
	if (sequence != NULL) {
		int tool = panel_timeline->tool;
        if (event->button() == Qt::MiddleButton) {
            tool = TIMELINE_TOOL_HAND;
            panel_timeline->creating = false;
        } else if (event->button() == Qt::RightButton) {
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
			switch (panel_timeline->creating_object) {
			case ADD_OBJ_TITLE:
			case ADD_OBJ_SOLID:
			case ADD_OBJ_BARS:
				comp = -1;
				break;
			case ADD_OBJ_TONE:
			case ADD_OBJ_NOISE:
			case ADD_OBJ_AUDIO:
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
								} else if (panel_timeline->tool == TIMELINE_TOOL_POINTER && panel_timeline->transition_select != TA_NO_TRANSITION) {
									panel_timeline->deselect_area(clip->timeline_in, clip->timeline_out, clip->track);

									for (int i=0;i<clip->linked.size();i++) {
										Clip* link = sequence->clips.at(clip->linked.at(i));
										panel_timeline->deselect_area(link->timeline_in, link->timeline_out, link->track);
									}

									Selection s;
									s.track = clip->track;

									if (panel_timeline->transition_select == TA_OPENING_TRANSITION && clip->get_opening_transition() != NULL) {
										s.in = clip->timeline_in;
										if (clip->get_opening_transition()->secondary_clip != NULL) s.in -= clip->get_opening_transition()->get_true_length();
										s.out = clip->timeline_in + clip->get_opening_transition()->get_true_length();
									} else if (panel_timeline->transition_select == TA_CLOSING_TRANSITION && clip->get_closing_transition() != NULL) {
										s.in = clip->timeline_out - clip->get_closing_transition()->get_true_length();
										s.out = clip->timeline_out;
										if (clip->get_closing_transition()->secondary_clip != NULL) s.out += clip->get_closing_transition()->get_true_length();
									}
									sequence->selections.append(s);
								}
							} else {
								// if "shift" is not down
								if (!shift) {
									sequence->selections.clear();
								}

								Selection s;

								s.in = clip->timeline_in;
								s.out = clip->timeline_out;

								if (panel_timeline->tool == TIMELINE_TOOL_POINTER) {
									if (panel_timeline->transition_select == TA_OPENING_TRANSITION) {
										s.out = clip->timeline_in + clip->get_opening_transition()->get_true_length();
										if (clip->get_opening_transition()->secondary_clip != NULL) s.in -= clip->get_opening_transition()->get_true_length();
									}

									if (panel_timeline->transition_select == TA_CLOSING_TRANSITION) {
										s.in = clip->timeline_out - clip->get_closing_transition()->get_true_length();
										if (clip->get_closing_transition()->secondary_clip != NULL) s.out += clip->get_closing_transition()->get_true_length();
									}
								}

								s.track = clip->track;
								sequence->selections.append(s);

								if (config.select_also_seeks) {
									panel_sequence_viewer->seek(clip->timeline_in);
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
					update_ui(false);
				}
			}
				break;
			case TIMELINE_TOOL_HAND:
				panel_timeline->hand_moving = true;
				panel_timeline->drag_x_start = pos.x();
				panel_timeline->drag_y_start = pos.y();
				break;
			case TIMELINE_TOOL_EDIT:
				if (config.edit_tool_also_seeks) panel_sequence_viewer->seek(panel_timeline->drag_frame_start);
				panel_timeline->selecting = true;
				break;
			case TIMELINE_TOOL_RAZOR:
			{
				panel_timeline->splitting = true;
				panel_timeline->split_tracks.append(panel_timeline->drag_track_start);
				update_ui(false);
			}
				break;
			case TIMELINE_TOOL_TRANSITION:
			{
				if (panel_timeline->transition_tool_pre_clip > -1) {
					panel_timeline->transition_tool_init = true;
				}
			}
				break;
			}
		}
	}
}

void make_room_for_transition(ComboAction* ca, Clip* c, int type, long transition_start, long transition_end, bool delete_old_transitions) {
	// make room for transition
	if (type == TA_OPENING_TRANSITION) {
		if (delete_old_transitions && c->get_opening_transition() != NULL) {
			ca->append(new DeleteTransitionCommand(c->sequence, c->opening_transition));
		}
		if (c->get_closing_transition() != NULL) {
			if (transition_end >= c->timeline_out) {
				ca->append(new DeleteTransitionCommand(c->sequence, c->closing_transition));
			} else if (transition_end > c->timeline_out - c->get_closing_transition()->get_true_length()) {
				ca->append(new ModifyTransitionCommand(c, TA_CLOSING_TRANSITION, c->timeline_out - transition_end));
			}
		}
	} else {
		if (delete_old_transitions && c->get_closing_transition() != NULL) {
			ca->append(new DeleteTransitionCommand(c->sequence, c->closing_transition));
		}
		if (c->get_opening_transition() != NULL) {
			if (transition_start <= c->timeline_in) {
				ca->append(new DeleteTransitionCommand(c->sequence, c->opening_transition));
			} else if (transition_start < c->timeline_in + c->get_opening_transition()->get_true_length()) {
				ca->append(new ModifyTransitionCommand(c, TA_OPENING_TRANSITION, transition_start - c->timeline_in));
			}
		}
	}
}

void TimelineWidget::mouseReleaseEvent(QMouseEvent *event) {
	QToolTip::hideText();
	if (sequence != NULL) {
		bool alt = (event->modifiers() & Qt::AltModifier);
		bool shift = (event->modifiers() & Qt::ShiftModifier);
		bool ctrl = (event->modifiers() & Qt::ControlModifier);

		if (event->button() == Qt::LeftButton) {
			ComboAction* ca = new ComboAction();
			bool push_undo = false;

			if (panel_timeline->creating) {
				if (panel_timeline->ghosts.size() > 0) {
					const Ghost& g = panel_timeline->ghosts.at(0);

					if (panel_timeline->creating_object == ADD_OBJ_AUDIO) {
						mainWindow->statusBar()->clearMessage();
						panel_sequence_viewer->cue_recording(qMin(g.in, g.out), qMax(g.in, g.out), g.track);
						panel_timeline->creating = false;
					} else if (g.in != g.out) {
						Clip* c = new Clip(sequence);
						c->media = NULL;
						c->timeline_in = qMin(g.in, g.out);
						c->timeline_out = qMax(g.in, g.out);
						c->clip_in = 0;
						c->color_r = 192;
						c->color_g = 192;
						c->color_b = 64;
						c->track = g.track;

						if (ctrl) {
							insert_clips(ca);
						} else {
							Selection s;
							s.in = c->timeline_in;
							s.out = c->timeline_out;
							s.track = c->track;
							QVector<Selection> areas;
							areas.append(s);
							panel_timeline->delete_areas_and_relink(ca, areas);
						}

						QVector<Clip*> add;
						add.append(c);
						ca->append(new AddClipCommand(sequence, add));

						if (c->track < 0) {
							// default video effects (before custom effects)
							c->effects.append(create_effect(c, get_internal_meta(EFFECT_INTERNAL_TRANSFORM, EFFECT_TYPE_EFFECT)));
							//c->media_type = MEDIA_TYPE_SOLID;
						}

						switch (panel_timeline->creating_object) {
						case ADD_OBJ_TITLE:
							c->name = "Title";
							c->effects.append(create_effect(c, get_internal_meta(EFFECT_INTERNAL_TEXT, EFFECT_TYPE_EFFECT)));
							break;
						case ADD_OBJ_SOLID:
							c->name = "Solid Color";
							c->effects.append(create_effect(c, get_internal_meta(EFFECT_INTERNAL_SOLID, EFFECT_TYPE_EFFECT)));
							break;
						case ADD_OBJ_BARS:
						{
							c->name = "Bars";
							Effect* e = create_effect(c, get_internal_meta(EFFECT_INTERNAL_SOLID, EFFECT_TYPE_EFFECT));
							e->row(0)->field(0)->set_combo_index(1);
							c->effects.append(e);
						}
							break;
						case ADD_OBJ_TONE:
							c->name = "Tone";
							c->effects.append(create_effect(c, get_internal_meta(EFFECT_INTERNAL_TONE, EFFECT_TYPE_EFFECT)));
							break;
						case ADD_OBJ_NOISE:
							c->name = "Noise";
							c->effects.append(create_effect(c, get_internal_meta(EFFECT_INTERNAL_NOISE, EFFECT_TYPE_EFFECT)));
							break;
						}

						if (c->track >= 0) {
							// default audio effects (after custom effects)
							c->effects.append(create_effect(c, get_internal_meta(EFFECT_INTERNAL_VOLUME, EFFECT_TYPE_EFFECT)));
							c->effects.append(create_effect(c, get_internal_meta(EFFECT_INTERNAL_PAN, EFFECT_TYPE_EFFECT)));
							//c->media_type = MEDIA_TYPE_TONE;
						}

						push_undo = true;

						if (!shift) {
							panel_timeline->creating = false;
						}
					}
				}
			} else if (panel_timeline->moving_proc) {
				if (panel_timeline->ghosts.size() > 0) {
					 const Ghost& first_ghost = panel_timeline->ghosts.at(0);

					 // if we were RIPPLING, move all the clips
					 if (panel_timeline->tool == TIMELINE_TOOL_RIPPLE) {
						 long ripple_length, ripple_point;

						 // ripple_length becomes the length/number of frames we trimmed
						 // ripple point becomes the point to ripple (i.e. the point after or before which we move every clip)
						 if (panel_timeline->trim_in_point) {
							 ripple_length = first_ghost.old_in - first_ghost.in;
							 ripple_point = first_ghost.old_in;

							 for (int i=0;i<sequence->selections.size();i++) {
								 sequence->selections[i].in += ripple_length;
								 sequence->selections[i].out += ripple_length;
							 }
						 } else {
							 // if we're trimming an out-point
							 ripple_length = first_ghost.old_out - panel_timeline->ghosts.at(0).out;
							 ripple_point = first_ghost.old_out;
						 }
						 QVector<int> ignore_clips;
						 for (int i=0;i<panel_timeline->ghosts.size();i++) {
							 const Ghost& g = panel_timeline->ghosts.at(i);

							 // push rippled clips forward if necessary
							 if (panel_timeline->trim_in_point) {
								 ignore_clips.append(g.clip);
								 panel_timeline->ghosts[i].in += ripple_length;
								 panel_timeline->ghosts[i].out += ripple_length;
							 }

							 long comp_point = panel_timeline->trim_in_point ? g.old_in : g.old_out;
							 ripple_point = qMin(ripple_point, comp_point);
						 }
						 if (!panel_timeline->trim_in_point) ripple_length = -ripple_length;

						 ripple_clips(ca, sequence, ripple_point, ripple_length, ignore_clips);
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
						 // INSERT if holding ctrl
						 if (panel_timeline->tool == TIMELINE_TOOL_POINTER && ctrl) {
							 insert_clips(ca);
						 } else if (panel_timeline->tool == TIMELINE_TOOL_POINTER || panel_timeline->tool == TIMELINE_TOOL_SLIDE) {
							// move clips
							 QVector<Selection> delete_areas;
							 for (int i=0;i<panel_timeline->ghosts.size();i++) {
								 // step 1 - set clips that are moving to "undeletable" (to avoid step 2 deleting any part of them)
								 const Ghost& g = panel_timeline->ghosts.at(i);

								 sequence->clips.at(g.clip)->undeletable = true;
								 if (g.transition != NULL) {
									 g.transition->parent_clip->undeletable = true;
									 if (g.transition->secondary_clip != NULL) g.transition->secondary_clip->undeletable = true;
								 }

								 Selection s;
								 s.in = g.in;
								 s.out = g.out;
								 s.track = g.track;
								 delete_areas.append(s);
							 }
							 panel_timeline->delete_areas_and_relink(ca, delete_areas);
							 for (int i=0;i<panel_timeline->ghosts.size();i++) {
								 const Ghost& g = panel_timeline->ghosts.at(i);
								 sequence->clips.at(g.clip)->undeletable = false;
								 if (g.transition != NULL) {
									 g.transition->parent_clip->undeletable = false;
									 if (g.transition->secondary_clip != NULL) g.transition->secondary_clip->undeletable = false;
								 }
							 }
						 }
						 for (int i=0;i<panel_timeline->ghosts.size();i++) {
							 Ghost& g = panel_timeline->ghosts[i];

							 // step 3 - move clips
							 Clip* c = sequence->clips.at(g.clip);
							 if (g.transition == NULL) {
								 move_clip(ca, c, (g.in - g.old_in), (g.out - g.old_out), (g.clip_in - g.old_clip_in), (g.track - g.old_track), true, true);

								 // adjust transitions if we need to
								 long new_clip_length = (g.out - g.in);
								 if (c->get_opening_transition() != NULL) {
									 long max_open_length = new_clip_length;
									 if (c->get_closing_transition() != NULL && !panel_timeline->trim_in_point) {
										 max_open_length -= c->get_closing_transition()->get_true_length();
									 }
									 if (max_open_length <= 0) {
										 ca->append(new DeleteTransitionCommand(c->sequence, c->opening_transition));
									 } else if (c->get_opening_transition()->get_true_length() > max_open_length) {
										 ca->append(new ModifyTransitionCommand(c, TA_OPENING_TRANSITION, max_open_length));
									 }
								 }
								 if (c->get_closing_transition() != NULL) {
									 long max_open_length = new_clip_length;
									 if (c->get_opening_transition() != NULL && panel_timeline->trim_in_point) {
										 max_open_length -= c->get_opening_transition()->get_true_length();
									 }
									 if (max_open_length <= 0) {
										 ca->append(new DeleteTransitionCommand(c->sequence, c->closing_transition));
									 } else if (c->get_closing_transition()->get_true_length() > max_open_length) {
										 ca->append(new ModifyTransitionCommand(c, TA_CLOSING_TRANSITION, max_open_length));
									 }
								 }
							 } else {
								 bool is_opening_transition = (g.transition == c->get_opening_transition());
								 long new_transition_length = g.out - g.in;
								 if (g.transition->secondary_clip != NULL) new_transition_length >>= 1;
								 ca->append(new ModifyTransitionCommand(c, is_opening_transition ? TA_OPENING_TRANSITION : TA_CLOSING_TRANSITION, new_transition_length));

								 long clip_length = c->getLength();

								 if (g.transition->secondary_clip != NULL) {
									 if (g.in != g.old_in && !g.trimming) {
										 long movement = g.in - g.old_in;
										 move_clip(ca, g.transition->parent_clip, movement, 0, movement, 0, false, true);
										 move_clip(ca, g.transition->secondary_clip, 0, movement, 0, 0, false, true);
									 }
								 } else if (is_opening_transition) {
									 if (g.in != g.old_in) {
										 // if transition is going to make the clip bigger, make the clip bigger
										 move_clip(ca, c, (g.in - g.old_in), 0, (g.clip_in - g.old_clip_in), 0, true, true);
										 clip_length -= (g.in - g.old_in);
									 }

									 make_room_for_transition(ca, c, TA_OPENING_TRANSITION, g.in, g.out, false);
								 } else {
									 if (g.out != g.old_out) {
										 // if transition is going to make the clip bigger, make the clip bigger
										 move_clip(ca, c, 0, (g.out - g.old_out), 0, 0, true, true);
										 clip_length += (g.out - g.old_out);
									 }

									 make_room_for_transition(ca, c, TA_CLOSING_TRANSITION, g.in, g.out, false);
								 }
							 }
						 }
					 }
					 push_undo = true;
				}
			} else if (panel_timeline->selecting || panel_timeline->rect_select_proc) {
			} else if (panel_timeline->transition_tool_proc) {
				const Ghost& g = panel_timeline->ghosts.at(0);

				if (g.in != g.out) {
					long transition_start = qMin(g.in, g.out);
					long transition_end = qMax(g.in, g.out);

					Clip* pre = sequence->clips.at(g.clip);
					Clip* post = pre;

					make_room_for_transition(ca, pre, panel_timeline->transition_tool_type, transition_start, transition_end, true);

					if (panel_timeline->transition_tool_post_clip > -1) {
						post = sequence->clips.at(panel_timeline->transition_tool_post_clip);
						int opposite_type = (panel_timeline->transition_tool_type == TA_OPENING_TRANSITION) ? TA_CLOSING_TRANSITION : TA_OPENING_TRANSITION;
						make_room_for_transition(
									ca,
									post,
									opposite_type,
									transition_start,
									transition_end,
									true
								);

						if (panel_timeline->transition_tool_type == TA_CLOSING_TRANSITION) {
							// swap
							Clip* temp = pre;
							pre = post;
							post = temp;
						}
					}

					if (transition_start < post->timeline_in || transition_end > pre->timeline_out) {
						// delete shit over there and extend timeline in
						QVector<Selection> areas;
						Selection s;
						s.track = post->track;

						bool move_post = false;
						bool move_pre = false;

						if (transition_start < post->timeline_in) {
							s.in = transition_start;
							s.out = post->timeline_in;
							areas.append(s);
							move_post = true;
						}
						if (transition_end > pre->timeline_out) {
							s.in = pre->timeline_out;
							s.out = transition_end;
							areas.append(s);
							move_pre = true;
						}

						panel_timeline->delete_areas_and_relink(ca, areas);

						if (move_post) move_clip(ca, post, qMin(transition_start, post->timeline_in), post->timeline_out, post->clip_in - (post->timeline_in - transition_start), post->track);
						if (move_pre) move_clip(ca, pre, pre->timeline_in, qMax(transition_end, pre->timeline_out), pre->clip_in, pre->track);
					}

					if (panel_timeline->transition_tool_post_clip > -1) {
						ca->append(new AddTransitionCommand(pre, post, NULL, panel_timeline->transition_tool_meta, TA_OPENING_TRANSITION, transition_end - pre->timeline_in));
					} else {
						ca->append(new AddTransitionCommand(pre, NULL, NULL, panel_timeline->transition_tool_meta, panel_timeline->transition_tool_type, transition_end - transition_start));
					}

					push_undo = true;
				}
			} else if (panel_timeline->splitting) {
				bool split = false;
				for (int i=0;i<panel_timeline->split_tracks.size();i++) {
					int split_index = getClipIndexFromCoords(panel_timeline->drag_frame_start, panel_timeline->split_tracks.at(i));
					if (split_index > -1 && panel_timeline->split_clip_and_relink(ca, split_index, panel_timeline->drag_frame_start, !alt)) {
						split = true;
					}
				}
				if (split) {
					push_undo = true;
				}
				panel_timeline->split_cache.clear();
			}

			// remove duplicate selections
			panel_timeline->clean_up_selections(sequence->selections);

			if (selection_command != NULL) {
				selection_command->new_data = sequence->selections;
				ca->append(selection_command);
				selection_command = NULL;
				push_undo = true;
			}

			if (push_undo) {
				undo_stack.push(ca);
			} else {
				delete ca;
			}

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
			panel_timeline->transition_tool_init = false;
			panel_timeline->transition_tool_proc = false;
			pre_clips.clear();
			post_clips.clear();

			update_ui(true);
		}
        panel_timeline->hand_moving = false;
	}
}

void TimelineWidget::init_ghosts() {
	for (int i=0;i<panel_timeline->ghosts.size();i++) {
		Ghost& g = panel_timeline->ghosts[i];
		Clip* c = sequence->clips.at(g.clip);

		g.track = g.old_track = c->track;
		g.clip_in = g.old_clip_in = c->clip_in;

		if (panel_timeline->tool == TIMELINE_TOOL_SLIP) {
			g.clip_in = g.old_clip_in = c->get_clip_in_with_transition();
			g.in = g.old_in = c->get_timeline_in_with_transition();
			g.out = g.old_out = c->get_timeline_out_with_transition();
			g.ghost_length = g.old_out - g.old_in;
		} else if (g.transition == NULL) {
			// this ghost is for a clip
			g.in = g.old_in = c->timeline_in;
			g.out = g.old_out = c->timeline_out;
			g.ghost_length = g.old_out - g.old_in;
		} else if (g.transition == c->get_opening_transition()) {
			g.in = g.old_in = c->get_timeline_in_with_transition();
			g.ghost_length = c->get_opening_transition()->get_length();
			g.out = g.old_out = g.in + g.ghost_length;
		} else if (g.transition == c->get_closing_transition()) {
			g.out = g.old_out = c->get_timeline_out_with_transition();
			g.ghost_length = c->get_closing_transition()->get_length();
			g.in = g.old_in = g.out - g.ghost_length;
			g.clip_in = g.old_clip_in = c->clip_in + c->getLength() - c->get_closing_transition()->get_true_length();
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

void validate_transitions(Clip* c, int transition_type, long& frame_diff) {
	long validator;

	if (transition_type == TA_OPENING_TRANSITION) {
		// prevent from going below 0 on the timeline
		validator = c->timeline_in + frame_diff;
		if (validator < 0) frame_diff -= validator;

		// prevent from going below 0 for the media
		validator = c->clip_in + frame_diff;
		if (validator < 0) frame_diff -= validator;

		// prevent transition from exceeding media length
		validator -= c->getMaximumLength();
		if (validator > 0) frame_diff -= validator;
	} else {
		// prevent from going below 0 on the timeline
		validator = c->timeline_out + frame_diff;
		if (validator < 0) frame_diff -= validator;

		// prevent from going below 0 for the media
		validator = c->clip_in + c->getLength() + frame_diff;
		if (validator < 0) frame_diff -= validator;

		// prevent transition from exceeding media length
		validator -= c->getMaximumLength();
		if (validator > 0) frame_diff -= validator;
	}
}

void TimelineWidget::update_ghosts(const QPoint& mouse_pos, bool lock_frame) {
	int effective_tool = panel_timeline->tool;
	if (panel_timeline->importing || panel_timeline->creating) effective_tool = TIMELINE_TOOL_POINTER;

	int mouse_track = getTrackFromScreenPoint(mouse_pos.y());
	long frame_diff = (lock_frame) ? 0 : panel_timeline->getTimelineFrameFromScreenPoint(mouse_pos.x()) - panel_timeline->drag_frame_start;
	int track_diff = ((effective_tool == TIMELINE_TOOL_SLIDE || panel_timeline->transition_select != TA_NO_TRANSITION) && !panel_timeline->importing) ? 0 : mouse_track - panel_timeline->drag_track_start;
	long validator;
	long earliest_in_point = LONG_MAX;

	// first try to snap
	long fm;

	if (effective_tool != TIMELINE_TOOL_SLIP) {
		// slipping doesn't move the clips so we don't bother snapping for it
		for (int i=0;i<panel_timeline->ghosts.size();i++) {
			const Ghost& g = panel_timeline->ghosts.at(i);
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

	bool clips_are_movable = (effective_tool == TIMELINE_TOOL_POINTER || effective_tool == TIMELINE_TOOL_SLIDE);

	// validate ghosts
	long temp_frame_diff = frame_diff; // cache to see if we change it (thus cancelling any snap)
	for (int i=0;i<panel_timeline->ghosts.size();i++) {
		const Ghost& g = panel_timeline->ghosts.at(i);
		Clip* c = NULL;
		if (g.clip != -1) c = sequence->clips.at(g.clip);

		const FootageStream* ms = NULL;
		if (g.clip != -1 && c->media != NULL && c->media->get_type() == MEDIA_TYPE_FOOTAGE) {
			ms = c->media->to_footage()->get_stream_from_file_index(c->track < 0, c->media_stream);
		}

		// validate ghosts for trimming
		if (panel_timeline->creating) {
			// i feel like we might need something here but we haven't so far?
		} else if (effective_tool == TIMELINE_TOOL_SLIP) {
			if ((c->media != NULL && c->media->get_type() == MEDIA_TYPE_SEQUENCE)
					|| (ms != NULL && !ms->infinite_length)) {
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
				if (effective_tool != TIMELINE_TOOL_RIPPLE) {
					validator = g.old_in + frame_diff;
					if (validator < 0) frame_diff -= validator;
				}

				// prevent clip_in from going below 0
				if ((c->media != NULL && c->media->get_type() == MEDIA_TYPE_SEQUENCE)
						|| (ms != NULL && !ms->infinite_length)) {
					validator = g.old_clip_in + frame_diff;
					if (validator < 0) frame_diff -= validator;
				}
			} else {
				// prevent clip length from being less than 1 frame long
				validator = g.ghost_length + frame_diff;
				if (validator < 1) frame_diff += (1 - validator);

				// prevent clip length exceeding media length
				if ((c->media != NULL && c->media->get_type() == MEDIA_TYPE_SEQUENCE)
						|| (ms != NULL && !ms->infinite_length)) {
					validator = g.old_clip_in + g.ghost_length + frame_diff;
					if (validator > g.media_length) frame_diff -= validator - g.media_length;
				}
			}

			// prevent dual transition from going below 0 on the primary or media length on the secondary
			if (g.transition != NULL && g.transition->secondary_clip != NULL) {
				Clip* otc = g.transition->parent_clip;
				Clip* ctc = g.transition->secondary_clip;

				if (g.trim_in) {
					frame_diff -= g.transition->get_true_length();
				} else {
					frame_diff += g.transition->get_true_length();
				}

				validate_transitions(otc, TA_OPENING_TRANSITION, frame_diff);
				validate_transitions(ctc, TA_CLOSING_TRANSITION, frame_diff);

				frame_diff = -frame_diff;
				validate_transitions(otc, TA_OPENING_TRANSITION, frame_diff);
				validate_transitions(ctc, TA_CLOSING_TRANSITION, frame_diff);
				frame_diff = -frame_diff;

				if (g.trim_in) {
					frame_diff += g.transition->get_true_length();
				} else {
					frame_diff -= g.transition->get_true_length();
				}
			}

			// ripple ops
			if (effective_tool == TIMELINE_TOOL_RIPPLE) {
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
				if (g.transition->secondary_clip != NULL) {
					// prevent dual transitions from going below 0 on the primary or above media length on the secondary

					validator = g.transition->parent_clip->get_clip_in_with_transition() + frame_diff;
					if (validator < 0) frame_diff -= validator;

					validator = g.transition->secondary_clip->get_timeline_out_with_transition() - g.transition->secondary_clip->get_timeline_in_with_transition() - g.transition->get_length() + g.transition->secondary_clip->get_clip_in_with_transition() + frame_diff;
					if (validator < 0) frame_diff -= validator;

					validator = g.transition->parent_clip->clip_in + frame_diff - g.transition->parent_clip->getMaximumLength() + g.transition->get_true_length();
					if (validator > 0) frame_diff -= validator;

					validator = g.transition->secondary_clip->get_timeline_out_with_transition() - g.transition->secondary_clip->get_timeline_in_with_transition() + g.transition->secondary_clip->get_clip_in_with_transition() + frame_diff - g.transition->secondary_clip->getMaximumLength();
					if (validator > 0) frame_diff -= validator;
				} else {
					// prevent clip_in from going below 0
					if (c->media->get_type() == MEDIA_TYPE_SEQUENCE
							|| (ms != NULL && !ms->infinite_length)) {
						validator = g.old_clip_in + frame_diff;
						if (validator < 0) frame_diff -= validator;
					}

					// prevent clip length exceeding media length
					if (c->media->get_type() == MEDIA_TYPE_SEQUENCE
							|| (ms != NULL && !ms->infinite_length)) {
						validator = g.old_clip_in + g.ghost_length + frame_diff;
						if (validator > g.media_length) frame_diff -= validator - g.media_length;
					}
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
		} else if (effective_tool == TIMELINE_TOOL_TRANSITION) {
			if (panel_timeline->transition_tool_post_clip == -1) {
				validate_transitions(c, panel_timeline->transition_tool_type, frame_diff);
			} else {
				Clip* otc = c; // open transition clip
				Clip* ctc = sequence->clips.at(panel_timeline->transition_tool_post_clip); // close transition clip

				if (panel_timeline->transition_tool_type == TA_CLOSING_TRANSITION) {
					// swap
					Clip* temp = otc;
					otc = ctc;
					ctc = temp;
				}

				// always gets a positive frame_diff
				validate_transitions(otc, TA_OPENING_TRANSITION, frame_diff);
				validate_transitions(ctc, TA_CLOSING_TRANSITION, frame_diff);

				// always gets a negative frame_diff
				frame_diff = -frame_diff;
				validate_transitions(otc, TA_OPENING_TRANSITION, frame_diff);
				validate_transitions(ctc, TA_CLOSING_TRANSITION, frame_diff);
				frame_diff = -frame_diff;
			}
		}
	}
	if (temp_frame_diff != frame_diff) panel_timeline->snapped = false;

	// apply changes to ghosts
	for (int i=0;i<panel_timeline->ghosts.size();i++) {
		Ghost& g = panel_timeline->ghosts[i];

		if (effective_tool == TIMELINE_TOOL_SLIP) {
			g.clip_in = g.old_clip_in - frame_diff;
		} else if (g.trimming) {
			long ghost_diff = frame_diff;

			// prevent trimming clips from overlapping each other
			for (int j=0;j<panel_timeline->ghosts.size();j++) {
				const Ghost& comp = panel_timeline->ghosts.at(j);
				if (i != j && g.track == comp.track) {
					long validator;
					if (g.trim_in && comp.out < g.out) {
						validator = (g.old_in + ghost_diff) - comp.out;
						if (validator < 0) ghost_diff -= validator;
					} else if (comp.in > g.in) {
						validator = (g.old_out + ghost_diff) - comp.in;
						if (validator > 0) ghost_diff -= validator;
					}
				}
			}

			// apply changes
			if (g.transition != NULL && g.transition->secondary_clip != NULL) {
				if (g.trim_in) ghost_diff = -ghost_diff;
				g.in = g.old_in - ghost_diff;
				g.out = g.old_out + ghost_diff;
			} else if (g.trim_in) {
				g.in = g.old_in + ghost_diff;
				g.clip_in = g.old_clip_in + ghost_diff;
			} else {
				g.out = g.old_out + ghost_diff;
			}
		} else if (clips_are_movable) {
			g.track = g.old_track;
			g.in = g.old_in + frame_diff;
			g.out = g.old_out + frame_diff;

			if (g.transition != NULL && g.transition == sequence->clips.at(g.clip)->get_opening_transition()) {
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
		} else if (effective_tool == TIMELINE_TOOL_TRANSITION) {
			if (panel_timeline->transition_tool_post_clip > -1) {
				g.in = g.old_in - frame_diff;
				g.out = g.old_out + frame_diff;
			} else if (panel_timeline->transition_tool_type == TA_OPENING_TRANSITION) {
				g.out = g.old_out + frame_diff;
			} else {
				g.in = g.old_in + frame_diff;
			}
		}

		earliest_in_point = qMin(earliest_in_point, g.in);
	}

	// apply changes to selections
	if (effective_tool != TIMELINE_TOOL_SLIP && !panel_timeline->importing && !panel_timeline->creating) {
		for (int i=0;i<sequence->selections.size();i++) {
			Selection& s = sequence->selections[i];
			if (panel_timeline->trim_target > -1) {
				if (panel_timeline->trim_in_point) {
					s.in = s.old_in + frame_diff;
				} else {
					s.out = s.old_out + frame_diff;
				}
			} else if (clips_are_movable) {
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

	if (panel_timeline->importing) {
		QToolTip::showText(mapToGlobal(mouse_pos), frame_to_timecode(earliest_in_point, config.timecode_view, sequence->frame_rate));
	} else {
		QString tip = ((frame_diff < 0) ? "-" : "+") + frame_to_timecode(qAbs(frame_diff), config.timecode_view, sequence->frame_rate);
		if (panel_timeline->trim_target > -1) {
			// find which clip is being moved
			const Ghost* g = NULL;
			for (int i=0;i<panel_timeline->ghosts.size();i++) {
				if (panel_timeline->ghosts.at(i).clip == panel_timeline->trim_target) {
					g = &panel_timeline->ghosts.at(i);
					break;
				}
			}

			if (g != NULL) {
				tip += " Duration: ";
				long len = (g->old_out-g->old_in);
				if (panel_timeline->trim_in_point) {
					len -= frame_diff;
				} else {
					len += frame_diff;
				}
				tip += frame_to_timecode(len, config.timecode_view, sequence->frame_rate);
			}
		}
		QToolTip::showText(mapToGlobal(mouse_pos), tip);
	}
}

void TimelineWidget::mouseMoveEvent(QMouseEvent *event) {
	tooltip_timer.stop();
	if (sequence != NULL) {
		bool alt = (event->modifiers() & Qt::AltModifier);

		panel_timeline->cursor_frame = panel_timeline->getTimelineFrameFromScreenPoint(event->pos().x());
		panel_timeline->cursor_track = getTrackFromScreenPoint(event->pos().y());

		panel_timeline->move_insert = ((event->modifiers() & Qt::ControlModifier) && (panel_timeline->tool == TIMELINE_TOOL_POINTER || panel_timeline->importing || panel_timeline->creating));

		if (!panel_timeline->moving_init) track_resizing = false;

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
									Selection link_sel;
									link_sel.in = s.in;
									link_sel.out = s.out;
									link_sel.track = linked_tracks.at(k);
									sequence->selections.append(link_sel);
								}
							}

							break;
						}
					}
				}
			}

			if (config.edit_tool_also_seeks) {
				panel_sequence_viewer->seek(qMin(panel_timeline->drag_frame_start, panel_timeline->cursor_frame));
			} else {
				panel_timeline->repaint_timeline();
			}
		} else if (panel_timeline->hand_moving) {
			panel_timeline->block_repaints = true;
			panel_timeline->horizontalScrollBar->setValue(panel_timeline->horizontalScrollBar->value() + panel_timeline->drag_x_start - event->pos().x());
			scrollBar->setValue(scrollBar->value() + panel_timeline->drag_y_start - event->pos().y());
			panel_timeline->block_repaints = false;

			panel_timeline->repaint_timeline();

			panel_timeline->drag_x_start = event->pos().x();
			panel_timeline->drag_y_start = event->pos().y();
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
				update_ghosts(event->pos(), event->modifiers() & Qt::ShiftModifier);
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
						if (panel_timeline->tool == TIMELINE_TOOL_POINTER && (c->get_opening_transition() != NULL || c->get_closing_transition() != NULL)) {
							// check if any selections contain the whole clip or transition
							for (int j=0;j<sequence->selections.size();j++) {
								const Selection& s = sequence->selections.at(j);
								if (s.track == c->track) {
									if (selection_contains_transition(s, c, TA_OPENING_TRANSITION)) {
										g.transition = c->get_opening_transition();
										add = true;
										break;
									} else if (selection_contains_transition(s, c, TA_CLOSING_TRANSITION)) {
										g.transition = c->get_closing_transition();
										add = true;
										break;
									}
								}
							}
						}

						if (add && g.transition != NULL) {
							// check for duplicate transitions
							for (int j=0;j<panel_timeline->ghosts.size();j++) {
								if (panel_timeline->ghosts.at(j).transition == g.transition) {
									add = false;
									break;
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
					long axis = LONG_MAX;

					for (int i=0;i<panel_timeline->ghosts.size();i++) {
						Clip* c = sequence->clips.at(panel_timeline->ghosts.at(i).clip);
						if (panel_timeline->trim_in_point) {
							axis = qMin(axis, c->timeline_in);
						} else {
							axis = qMin(axis, c->timeline_out);
						}
					}

					for (int i=0;i<sequence->clips.size();i++) {
						Clip* c = sequence->clips.at(i);
						if (c != NULL && !panel_timeline->is_clip_selected(c, true)) {
							bool clip_is_post = (c->timeline_in >= axis);

							// see if this a clip on this track is already in the list, and if it's closer
							bool found = false;
							QVector<Clip*>& clip_list = clip_is_post ? post_clips : pre_clips;
							for (int j=0;j<clip_list.size();j++) {
								Clip* compare = clip_list.at(j);
								if (compare->track == c->track) {
									if ((!clip_is_post && compare->timeline_out < c->timeline_out)
											|| (clip_is_post && compare->timeline_in > c->timeline_in)) {
										clip_list[j] = c;
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

				// store selections
				selection_command = new SetSelectionsCommand(sequence);
				selection_command->old_data = sequence->selections;

				panel_timeline->moving_proc = true;
			}
			update_ui(false);
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
			update_ui(false);
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

				panel_timeline->repaint_timeline();
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
			panel_timeline->repaint_timeline();
		} else if (panel_timeline->tool == TIMELINE_TOOL_POINTER ||
				   panel_timeline->tool == TIMELINE_TOOL_RIPPLE ||
				   panel_timeline->tool == TIMELINE_TOOL_ROLLING) {
			QToolTip::hideText();

			QPoint pos = event->pos();

			int lim = 5;
			int mouse_track = getTrackFromScreenPoint(pos.y());
			long mouse_frame_lower = panel_timeline->getTimelineFrameFromScreenPoint(pos.x()-lim)-1;
			long mouse_frame_upper = panel_timeline->getTimelineFrameFromScreenPoint(pos.x()+lim)+1;
			bool found = false;
			bool cursor_contains_clip = false;
			int closeness = INT_MAX;
			int min_track = INT_MAX;
			int max_track = INT_MIN;
			panel_timeline->transition_select = TA_NO_TRANSITION;
			for (int i=0;i<sequence->clips.size();i++) {
				Clip* c = sequence->clips.at(i);
				if (c != NULL) {
					min_track = qMin(min_track, c->track);
					max_track = qMax(max_track, c->track);
					if (c->track == mouse_track) {
						if (panel_timeline->cursor_frame >= c->timeline_in &&
								panel_timeline->cursor_frame <= c->timeline_out) {
							cursor_contains_clip = true;

							tooltip_timer.start();
							tooltip_clip = i;

							if (c->get_opening_transition() != NULL && panel_timeline->cursor_frame <= c->timeline_in + c->get_opening_transition()->get_true_length()) {
								panel_timeline->transition_select = TA_OPENING_TRANSITION;
							} else if (c->get_closing_transition() != NULL && panel_timeline->cursor_frame >= c->timeline_out - c->get_closing_transition()->get_true_length()) {
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
							if (c->get_opening_transition() != NULL) {
								long transition_point = c->timeline_in + c->get_opening_transition()->get_true_length();

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
							if (c->get_closing_transition() != NULL) {
								long transition_point = c->timeline_out - c->get_closing_transition()->get_true_length();
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
					if (track >= min_track && track <= max_track) {
						int track_height = panel_timeline->calculate_track_height(track, -1);
						track_y += track_height;
						int y_test_value = (bottom_align) ? rect().bottom() - track_y : track_y;
						int test_range = 5;
						int mouse_pos = pos.y() + scroll;
						if (mouse_pos > y_test_value-test_range && mouse_pos < y_test_value+test_range) {
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
		} else if (panel_timeline->tool == TIMELINE_TOOL_TRANSITION) {
			if (panel_timeline->transition_tool_init) {
				if (panel_timeline->transition_tool_proc) {
					update_ghosts(event->pos(), event->modifiers() & Qt::ShiftModifier);
				} else {
					Clip* c = sequence->clips.at(panel_timeline->transition_tool_pre_clip);

					Ghost g;

					g.in = g.old_in = g.out = g.old_out = (panel_timeline->transition_tool_type == TA_OPENING_TRANSITION) ? c->timeline_in : c->timeline_out;
					g.track = c->track;
					g.clip = panel_timeline->transition_tool_pre_clip;
					g.media_stream = panel_timeline->transition_tool_type;
					g.trimming = false;

					panel_timeline->ghosts.append(g);

					panel_timeline->transition_tool_proc = true;
				}
			} else {
				int mouse_clip = getClipIndexFromCoords(panel_timeline->cursor_frame, panel_timeline->cursor_track);
				if (mouse_clip > -1) {
					Clip* c = sequence->clips.at(mouse_clip);
					if (same_sign(c->track, panel_timeline->transition_tool_side)) {
						panel_timeline->transition_tool_pre_clip = mouse_clip;
						long halfway = c->timeline_in + (c->getLength()/2);
						long between_range = getFrameFromScreenPoint(panel_timeline->zoom, TRANSITION_BETWEEN_RANGE) + 1;

						if (panel_timeline->cursor_frame > halfway) {
							panel_timeline->transition_tool_type = TA_CLOSING_TRANSITION;
						} else {
							panel_timeline->transition_tool_type = TA_OPENING_TRANSITION;
						}

						panel_timeline->transition_tool_post_clip = -1;
						if (panel_timeline->cursor_frame < c->timeline_in + between_range) {
							panel_timeline->transition_tool_post_clip = getClipIndexFromCoords(c->timeline_in-1, c->track);
						} else if (panel_timeline->cursor_frame > c->timeline_out - between_range) {
							panel_timeline->transition_tool_post_clip = getClipIndexFromCoords(c->timeline_out+1, c->track);
						}
					}
				} else {
					panel_timeline->transition_tool_pre_clip = -1;
					panel_timeline->transition_tool_post_clip = -1;
				}
			}

			panel_timeline->repaint_timeline();
		}
	}
}

void TimelineWidget::leaveEvent(QEvent*) {
	tooltip_timer.stop();
}

int color_brightness(int r, int g, int b) {
	return (0.2126*r + 0.7152*g + 0.0722*b);
}

void draw_waveform(Clip* clip, const FootageStream* ms, long media_length, QPainter *p, const QRect& clip_rect, int waveform_start, int waveform_limit, double zoom) {
	int divider = ms->audio_channels*2;
	int channel_height = clip_rect.height()/ms->audio_channels;

	for (int i=waveform_start;i<waveform_limit;i++) {
		int waveform_index = qFloor((((clip->clip_in + ((double) i/zoom))/media_length) * ms->audio_preview.size())/divider)*divider;

		if (clip->reverse) {
			waveform_index = ms->audio_preview.size() - waveform_index - (ms->audio_channels * 2);
		}

		for (int j=0;j<ms->audio_channels;j++) {
			int mid = (config.rectified_waveforms) ? clip_rect.top()+channel_height*(j+1) : clip_rect.top()+channel_height*j+(channel_height/2);
			int offset = waveform_index+(j*2);

			if ((offset + 1) < ms->audio_preview.size()) {
				qint8 min = (double)ms->audio_preview.at(offset) / 128.0 * (channel_height/2);
				qint8 max = (double)ms->audio_preview.at(offset+1) / 128.0 * (channel_height/2);

				if (config.rectified_waveforms)  {
					p->drawLine(clip_rect.left()+i, mid, clip_rect.left()+i, mid - (max - min));
				} else {
					p->drawLine(clip_rect.left()+i, mid+min, clip_rect.left()+i, mid+max);
				}
			}/* else {
				dout << "[WARNING] Tried to reach" << offset + 1 << ", limit:" << ms->audio_preview.size();
			}*/
		}
	}
}

void draw_transition(QPainter& p, Clip* c, const QRect& clip_rect, QRect& text_rect, int transition_type) {
	Transition* t = (transition_type == TA_OPENING_TRANSITION) ? c->get_opening_transition() : c->get_closing_transition();
	if (t != NULL) {
		QColor transition_color(255, 0, 0, 16);
		int transition_width = getScreenPointFromFrame(panel_timeline->zoom, t->get_true_length());
		int transition_height = clip_rect.height();
		int tr_y = clip_rect.y();
		int tr_x = 0;
		if (transition_type == TA_OPENING_TRANSITION) {
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
			bool draw_text = true;

			p.setPen(QColor(0, 0, 0, 96));
			if (t->secondary_clip == NULL) {
				if (transition_type == TA_OPENING_TRANSITION) {
					p.drawLine(transition_rect.bottomLeft(), transition_rect.topRight());
				} else {
					p.drawLine(transition_rect.topLeft(), transition_rect.bottomRight());
				}
			} else {
				if (transition_type == TA_OPENING_TRANSITION) {
					p.drawLine(QPoint(transition_rect.left(), transition_rect.center().y()), transition_rect.topRight());
					p.drawLine(QPoint(transition_rect.left(), transition_rect.center().y()), transition_rect.bottomRight());
					draw_text = false;
				} else {
					p.drawLine(QPoint(transition_rect.right(), transition_rect.center().y()), transition_rect.topLeft());
					p.drawLine(QPoint(transition_rect.right(), transition_rect.center().y()), transition_rect.bottomLeft());
				}
			}

			if (draw_text) {
				p.setPen(Qt::white);
				p.drawText(transition_text_rect, 0, t->meta->name, &transition_text_rect);
			}
		}
		p.setPen(Qt::black);
		p.drawRect(transition_rect);
	}

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

		int panel_height = TRACK_DEFAULT_HEIGHT;
		if (bottom_align) {
			for (int i=-1;i>=video_track_limit;i--) {
				panel_height += panel_timeline->calculate_track_height(i, -1);
			}
		} else {
			for (int i=0;i<=audio_track_limit;i++) {
				panel_height += panel_timeline->calculate_track_height(i, -1);
			}
		}
		if (bottom_align) {
			scrollBar->setMinimum(qMin(0, - panel_height + height()));
		} else {
			scrollBar->setMaximum(qMax(0, panel_height - height()));
		}

		for (int i=0;i<sequence->clips.size();i++) {
			Clip* clip = sequence->clips.at(i);
			if (clip != NULL && is_track_visible(clip->track)) {
				QRect clip_rect(panel_timeline->getTimelineScreenPointFromFrame(clip->timeline_in), getScreenPointFromTrack(clip->track), getScreenPointFromFrame(panel_timeline->zoom, clip->getLength()), panel_timeline->calculate_track_height(clip->track, -1));
				QRect text_rect(clip_rect.left() + CLIP_TEXT_PADDING, clip_rect.top() + CLIP_TEXT_PADDING, clip_rect.width() - CLIP_TEXT_PADDING - 1, clip_rect.height() - CLIP_TEXT_PADDING - 1);
				if (clip_rect.left() < width() && clip_rect.right() >= 0 && clip_rect.top() < height() && clip_rect.bottom() >= 0) {
					QRect actual_clip_rect = clip_rect;
					if (actual_clip_rect.x() < 0) actual_clip_rect.setX(0);
					if (actual_clip_rect.right() > width()) actual_clip_rect.setRight(width());
					if (actual_clip_rect.y() < 0) actual_clip_rect.setY(0);
					if (actual_clip_rect.bottom() > height()) actual_clip_rect.setBottom(height());
					p.fillRect(actual_clip_rect, (clip->enabled) ? QColor(clip->color_r, clip->color_g, clip->color_b) : QColor(96, 96, 96));

					int thumb_x = clip_rect.x() + 1;

					if (clip->media != NULL && clip->media->get_type() == MEDIA_TYPE_FOOTAGE) {
						bool draw_checkerboard = false;
						QRect checkerboard_rect(clip_rect);
						Footage* m = clip->media->to_footage();
						FootageStream* ms = m->get_stream_from_file_index(clip->track < 0, clip->media_stream);
						if (ms == NULL) {
							draw_checkerboard = true;
						} else if (ms->preview_done) {
							// draw top and tail triangles
							int triangle_size = TRACK_MIN_HEIGHT >> 2;
							if (!ms->infinite_length && clip_rect.width() > triangle_size) {
								p.setPen(Qt::NoPen);
								p.setBrush(QColor(80, 80, 80));
								if (clip->clip_in == 0
										&& clip_rect.x() + triangle_size > 0
										&& clip_rect.y() + triangle_size > 0
										&& clip_rect.x() < width()
										&& clip_rect.y() < height()) {
									const QPoint points[3] = {
										QPoint(clip_rect.x(), clip_rect.y()),
										QPoint(clip_rect.x() + triangle_size, clip_rect.y()),
										QPoint(clip_rect.x(), clip_rect.y() + triangle_size)
									};
									p.drawPolygon(points, 3);
									text_rect.setLeft(text_rect.left() + (triangle_size >> 2));
								}
								if (clip->timeline_out - clip->timeline_in + clip->clip_in == clip->getMaximumLength()
										&& clip_rect.right() - triangle_size < width()
										&& clip_rect.y() + triangle_size > 0
										&& clip_rect.right() > 0
										&& clip_rect.y() < height()) {
									const QPoint points[3] = {
										QPoint(clip_rect.right(), clip_rect.y()),
										QPoint(clip_rect.right() - triangle_size, clip_rect.y()),
										QPoint(clip_rect.right(), clip_rect.y() + triangle_size)
									};
									p.drawPolygon(points, 3);
									text_rect.setRight(text_rect.right() - (triangle_size >> 2));
								}
							}

							p.setBrush(Qt::NoBrush);

							// draw thumbnail/waveform
							long media_length = clip->getMaximumLength();

							if (clip->track < 0) {
								// draw thumbnail
								int thumb_y = p.fontMetrics().height()+CLIP_TEXT_PADDING+CLIP_TEXT_PADDING;
								if (thumb_x < width() && thumb_y < height()) {
									int space_for_thumb = clip_rect.width()-1;
									if (clip->get_opening_transition() != NULL) {
										int ot_width = getScreenPointFromFrame(panel_timeline->zoom, clip->get_opening_transition()->get_true_length());
										thumb_x += ot_width;
										space_for_thumb -= ot_width;
									}
									if (clip->get_closing_transition() != NULL) {
										space_for_thumb -= getScreenPointFromFrame(panel_timeline->zoom, clip->get_closing_transition()->get_true_length());
									}
									int thumb_height = clip_rect.height()-thumb_y;
									int thumb_width = (thumb_height*((double)ms->video_preview.width()/(double)ms->video_preview.height()));
									if (thumb_x + thumb_width >= 0
											&& thumb_height > thumb_y
											&& thumb_y + thumb_height >= 0
											&& space_for_thumb > MAX_TEXT_WIDTH) {
										int thumb_clip_width = qMin(thumb_width, space_for_thumb);
										p.drawImage(QRect(thumb_x, clip_rect.y()+thumb_y, thumb_clip_width, thumb_height), ms->video_preview, QRect(0, 0, thumb_clip_width*((double)ms->video_preview.width()/(double)thumb_width), ms->video_preview.height()));
									}
								}
								if (clip->timeline_out - clip->timeline_in + clip->clip_in > clip->getMaximumLength()) {
									draw_checkerboard = true;
									checkerboard_rect.setLeft(panel_timeline->getTimelineScreenPointFromFrame(clip->getMaximumLength() + clip->timeline_in - clip->clip_in));
								}
							} else if (clip_rect.height() > TRACK_MIN_HEIGHT) {
								// draw waveform
								p.setPen(QColor(80, 80, 80));

								int waveform_start = -qMin(clip_rect.x(), 0);
								int waveform_limit = qMin(clip_rect.width(), getScreenPointFromFrame(panel_timeline->zoom, media_length - clip->clip_in));

								if ((clip_rect.x() + waveform_limit) > width()) {
									waveform_limit -= (clip_rect.x() + waveform_limit - width());
								} else if (waveform_limit < clip_rect.width()) {
									draw_checkerboard = true;
									if (waveform_limit > 0) checkerboard_rect.setLeft(checkerboard_rect.left() + waveform_limit);
								}

								draw_waveform(clip, ms, media_length, &p, clip_rect, waveform_start, waveform_limit, panel_timeline->zoom);
							}
						}
						if (draw_checkerboard) {
							checkerboard_rect.setLeft(qMax(checkerboard_rect.left(), 0));
							checkerboard_rect.setRight(qMin(checkerboard_rect.right(), width()));
							checkerboard_rect.setTop(qMax(checkerboard_rect.top(), 0));
							checkerboard_rect.setBottom(qMin(checkerboard_rect.bottom(), height()));

							if (checkerboard_rect.left() < width()
									&& checkerboard_rect.right() >= 0
									&& checkerboard_rect.top() < height()
									&& checkerboard_rect.bottom() >= 0) {
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
					draw_transition(p, clip, clip_rect, text_rect, TA_OPENING_TRANSITION);
					draw_transition(p, clip, clip_rect, text_rect, TA_CLOSING_TRANSITION);

					// top left bevel
					p.setPen(Qt::white);
					if (clip_rect.x() >= 0 && clip_rect.x() < width()) p.drawLine(clip_rect.bottomLeft(), clip_rect.topLeft());
					if (clip_rect.y() >= 0 && clip_rect.y() < height()) p.drawLine(QPoint(qMax(0, clip_rect.left()), clip_rect.top()), QPoint(qMin(width(), clip_rect.right()), clip_rect.top()));

					// draw text
					if (text_rect.width() > MAX_TEXT_WIDTH && text_rect.right() > 0 && text_rect.left() < width()) {
						if (!clip->enabled) {
							p.setPen(Qt::gray);
						} else if (color_brightness(clip->color_r, clip->color_g, clip->color_b) > 160) {
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
					if (clip_rect.right() >= 0 && clip_rect.right() < width()) p.drawLine(clip_rect.bottomRight(), clip_rect.topRight());
					if (clip_rect.bottom() >= 0 && clip_rect.bottom() < height()) p.drawLine(QPoint(qMax(0, clip_rect.left()), clip_rect.bottom()), QPoint(qMin(width(), clip_rect.right()), clip_rect.bottom()));

					// draw transition tool
					if (panel_timeline->tool == TIMELINE_TOOL_TRANSITION && (panel_timeline->transition_tool_pre_clip == i || panel_timeline->transition_tool_post_clip == i)) {
						int type = panel_timeline->transition_tool_type;
						if (panel_timeline->transition_tool_post_clip == i) {
							// invert transition type
							type = (type == TA_CLOSING_TRANSITION) ? TA_OPENING_TRANSITION : TA_CLOSING_TRANSITION;
						}
						QRect transition_tool_rect = clip_rect;
						if (type == TA_CLOSING_TRANSITION) {
							if (panel_timeline->transition_tool_post_clip > -1) {
								transition_tool_rect.setLeft(transition_tool_rect.right() - TRANSITION_BETWEEN_RANGE);
							} else {
								transition_tool_rect.setLeft(transition_tool_rect.left() + (3*(transition_tool_rect.width()>>2)));
							}
						} else {
							if (panel_timeline->transition_tool_post_clip > -1) {
								transition_tool_rect.setWidth(TRANSITION_BETWEEN_RANGE);
							} else {
								transition_tool_rect.setWidth(transition_tool_rect.width()>>2);
							}
						}
						if (transition_tool_rect.left() < width() && transition_tool_rect.right() > 0) {
							if (transition_tool_rect.left() < 0) {
								transition_tool_rect.setLeft(0);
							}
							if (transition_tool_rect.right() > width()) {
								transition_tool_rect.setRight(width());
							}
							p.fillRect(transition_tool_rect, QColor(0, 0, 0, 128));
						}
					}
				}
			}
		}

		// Draw recording clip if recording if valid
		if (panel_sequence_viewer->is_recording_cued() && is_track_visible(panel_sequence_viewer->recording_track)) {
			int rec_track_x = panel_timeline->getTimelineScreenPointFromFrame(panel_sequence_viewer->recording_start);
			int rec_track_y = getScreenPointFromTrack(panel_sequence_viewer->recording_track);
			int rec_track_height = panel_timeline->calculate_track_height(panel_sequence_viewer->recording_track, -1);
			if (panel_sequence_viewer->recording_start != panel_sequence_viewer->recording_end) {
				QRect rec_rect(
							rec_track_x,
							rec_track_y,
							getScreenPointFromFrame(panel_timeline->zoom, panel_sequence_viewer->recording_end - panel_sequence_viewer->recording_start),
							rec_track_height
						 );
				p.setPen(QPen(QColor(96, 96, 96), 2));
				p.fillRect(rec_rect, QColor(192, 192, 192));
				p.drawRect(rec_rect);
			}
			QRect active_rec_rect(
						rec_track_x,
						rec_track_y,
						getScreenPointFromFrame(panel_timeline->zoom, panel_sequence_viewer->seq->playhead - panel_sequence_viewer->recording_start),
						rec_track_height
					 );
			p.setPen(QPen(QColor(192, 0, 0), 2));
			p.fillRect(active_rec_rect, QColor(255, 96, 96));
			p.drawRect(active_rec_rect);

			p.setPen(Qt::NoPen);

			if (!panel_sequence_viewer->playing) {
				int rec_marker_size = 6;
				int rec_track_midY = rec_track_y + (rec_track_height >> 1);
				p.setBrush(Qt::white);
				QPoint cue_marker[3] = {
					QPoint(rec_track_x, rec_track_midY - rec_marker_size),
					QPoint(rec_track_x + rec_marker_size, rec_track_midY),
					QPoint(rec_track_x, rec_track_midY + rec_marker_size)
				};
				p.drawPolygon(cue_marker, 3);
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
				p.setPen(Qt::NoPen);
				p.setBrush(Qt::NoBrush);
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
		if (!panel_timeline->ghosts.isEmpty()) {
			QVector<int> insert_points;
			long first_ghost = LONG_MAX;
			for (int i=0;i<panel_timeline->ghosts.size();i++) {
				const Ghost& g = panel_timeline->ghosts.at(i);
				first_ghost = qMin(first_ghost, g.in);
				if (is_track_visible(g.track)) {
					int ghost_x = panel_timeline->getTimelineScreenPointFromFrame(g.in);
					int ghost_y = getScreenPointFromTrack(g.track);
					int ghost_width = panel_timeline->getTimelineScreenPointFromFrame(g.out) - ghost_x - 1;
					int ghost_height = panel_timeline->calculate_track_height(g.track, -1) - 1;

					insert_points.append(ghost_y + (ghost_height>>1));

					p.setPen(QColor(255, 255, 0));
					for (int j=0;j<GHOST_THICKNESS;j++) {
						p.drawRect(ghost_x+j, ghost_y+j, ghost_width-j-j, ghost_height-j-j);
					}
				}
			}

			// draw insert indicator
			if (panel_timeline->move_insert && !insert_points.isEmpty()) {
				p.setBrush(Qt::white);
				p.setPen(Qt::NoPen);
				int insert_x = panel_timeline->getTimelineScreenPointFromFrame(first_ghost);
				int tri_size = TRACK_MIN_HEIGHT>>2;

				for (int i=0;i<insert_points.size();i++) {
					QPoint points[3] = {
						QPoint(insert_x, insert_points.at(i) - tri_size),
						QPoint(insert_x + tri_size, insert_points.at(i)),
						QPoint(insert_x, insert_points.at(i) + tri_size)
					};
					p.drawPolygon(points, 3);
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
		if (isLiveEditing() && is_track_visible(panel_timeline->cursor_track)) {
			int cursor_x = panel_timeline->getTimelineScreenPointFromFrame(panel_timeline->cursor_frame);
			int cursor_y = getScreenPointFromTrack(panel_timeline->cursor_track);

			p.setPen(Qt::gray);
			p.drawLine(cursor_x, cursor_y, cursor_x, cursor_y + panel_timeline->calculate_track_height(panel_timeline->cursor_track, -1));
		}
	}
}

void TimelineWidget::resizeEvent(QResizeEvent *event) {
	scrollBar->setPageStep(height());
}

bool TimelineWidget::is_track_visible(int track) {
	return (bottom_align == (track < 0));
}

// **************************************
// screen point <-> frame/track functions
// **************************************

int TimelineWidget::getTrackFromScreenPoint(int y) {
	y += scroll;
	if (bottom_align) {
		y = -(y - height());
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
	return (bottom_align) ? height() - y - scroll : y - scroll;
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

void TimelineWidget::setScroll(int s) {
	scroll = s;
	update();
}

void TimelineWidget::reveal_media() {
	panel_project->reveal_media(rc_reveal_media);
}
