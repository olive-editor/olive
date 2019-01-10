#include "timelineheader.h"

#include "mainwindow.h"
#include "panels/panels.h"
#include "panels/timeline.h"
#include "project/sequence.h"
#include "project/undo.h"
#include "panels/viewer.h"
#include "io/config.h"
#include "debug.h"

#include <QPainter>
#include <QMouseEvent>
#include <QScrollBar>
#include <QtMath>
#include <QMenu>
#include <QAction>

#define CLICK_RANGE 5
#define PLAYHEAD_SIZE 6
#define LINE_MIN_PADDING 50
#define SUBLINE_MIN_PADDING 50 // TODO play with this
#define MARKER_SIZE 4

bool center_scroll_to_playhead(QScrollBar* bar, double zoom, long playhead) {
	// returns true is the scroll was changed, false if not
	int target_scroll = qMin(bar->maximum(), qMax(0, getScreenPointFromFrame(zoom, playhead)-(bar->width()>>1)));
	if (target_scroll == bar->value()) {
		return false;
	}
	bar->setValue(target_scroll);
	return true;
}

TimelineHeader::TimelineHeader(QWidget *parent) :
	QWidget(parent),
	snapping(true),
	dragging(false),
	resizing_workarea(false),
	zoom(1),
	in_visible(0),
	fm(font()),
	dragging_markers(false),
	scroll(0)
{
	height_actual = fm.height();
	setCursor(Qt::ArrowCursor);
	setMouseTracking(true);
	setFocusPolicy(Qt::ClickFocus);
	show_text(true);

	setContextMenuPolicy(Qt::CustomContextMenu);
	connect(this, SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(show_context_menu(const QPoint &)));
}

void TimelineHeader::set_scroll(int s) {
	scroll = s;
	update();
}

long TimelineHeader::getHeaderFrameFromScreenPoint(int x) {
	return getFrameFromScreenPoint(zoom, x + scroll) + in_visible;
}

int TimelineHeader::getHeaderScreenPointFromFrame(long frame) {
	return getScreenPointFromFrame(zoom, frame - in_visible) - scroll;
}

void TimelineHeader::set_playhead(int mouse_x) {
	long frame = getHeaderFrameFromScreenPoint(mouse_x);
	if (snapping) panel_timeline->snap_to_timeline(&frame, false, true, true);
	if (frame != viewer->seq->playhead) {
		viewer->seek(frame);
	}
}

void TimelineHeader::set_visible_in(long i) {
	in_visible = i;
	update();
}

void TimelineHeader::set_in_point(long new_in) {
	long new_out = viewer->seq->workarea_out;
	if (new_out == new_in) {
		new_in--;
	} else if (new_out < new_in) {
		new_out = viewer->seq->getEndFrame();
	}

	undo_stack.push(new SetTimelineInOutCommand(viewer->seq, true, new_in, new_out));
	update_parents();
}

void TimelineHeader::set_out_point(long new_out) {
	long new_in = viewer->seq->workarea_in;
	if (new_out == new_in) {
		new_out++;
	} else if (new_in > new_out || new_in < 0) {
		new_in = 0;
	}

	undo_stack.push(new SetTimelineInOutCommand(viewer->seq, true, new_in, new_out));
	update_parents();
}

void TimelineHeader::set_scrollbar_max(QScrollBar* bar, long sequence_end_frame, int offset) {
	bar->setMaximum(qMax(0, getScreenPointFromFrame(zoom, sequence_end_frame) - offset));
}

void TimelineHeader::show_text(bool enable) {
	text_enabled = enable;
	if (enable) {
		setFixedHeight(height_actual*2);
	} else {
		setFixedHeight(height_actual);
	}
	update();
}

void TimelineHeader::mousePressEvent(QMouseEvent* event) {
	if (viewer->seq != NULL && event->buttons() & Qt::LeftButton) {
		if (resizing_workarea) {
			sequence_end = viewer->seq->getEndFrame();
		} else {
			bool shift = (event->modifiers() & Qt::ShiftModifier);
			bool clicked_on_marker = false;
			for (int i=0;i<viewer->seq->markers.size();i++) {
				int marker_pos = getHeaderScreenPointFromFrame(viewer->seq->markers.at(i).frame);
				if (event->pos().x() > marker_pos - MARKER_SIZE && event->pos().x() < marker_pos + MARKER_SIZE) {
					bool found = false;
					for (int j=0;j<selected_markers.size();j++) {
						if (selected_markers.at(j) == i) {
							if (shift) {
								selected_markers.removeAt(j);
							}
							found = true;
							break;
						}
					}
					if (!found) {
						if (!shift) {
							selected_markers.clear();
						}
						selected_markers.append(i);
					}
					clicked_on_marker = true;
					update();
					break;
				}
			}
			if (clicked_on_marker) {
				selected_marker_original_times.resize(selected_markers.size());
				for (int i=0;i<selected_markers.size();i++) {
					selected_marker_original_times[i] = viewer->seq->markers.at(selected_markers.at(i)).frame;
				}
				drag_start = event->pos().x();
				dragging_markers = true;
			} else {
				if (selected_markers.size() > 0) {
					selected_markers.clear();
					update();
				}
				set_playhead(event->pos().x());
			}
		}
		dragging = true;
	}
}

void TimelineHeader::mouseMoveEvent(QMouseEvent* event) {
	if (viewer->seq != NULL) {
		if (dragging) {
			if (resizing_workarea) {
				long frame = getHeaderFrameFromScreenPoint(event->pos().x());
				if (snapping) panel_timeline->snap_to_timeline(&frame, true, true, false);

				if (resizing_workarea_in) {
					temp_workarea_in = qMax(qMin(temp_workarea_out-1, frame), 0L);
				} else {
					temp_workarea_out = qMin(qMax(temp_workarea_in+1, frame), sequence_end);
				}

				update_parents();
			} else if (dragging_markers) {
				long frame_movement = getHeaderFrameFromScreenPoint(event->pos().x()) - getHeaderFrameFromScreenPoint(drag_start);

				// snap markers
				for (int i=0;i<selected_markers.size();i++) {
					long fm = selected_marker_original_times.at(i) + frame_movement;
					if (snapping && panel_timeline->snap_to_timeline(&fm, true, false, true)) {
						frame_movement = fm - selected_marker_original_times.at(i);
						break;
					}
				}

				// validate markers (ensure none go below 0)
				long validator;
				for (int i=0;i<selected_markers.size();i++) {
					validator = selected_marker_original_times.at(i) + frame_movement;
					if (validator < 0) {
						frame_movement -= validator;
					}
				}

				// move markers
				for (int i=0;i<selected_markers.size();i++) {
					viewer->seq->markers[selected_markers.at(i)].frame = selected_marker_original_times.at(i) + frame_movement;
				}

				update_parents();
			} else {
				set_playhead(event->pos().x());
			}
		} else {
			resizing_workarea = false;
			unsetCursor();
			if (viewer->seq != NULL && viewer->seq->using_workarea) {
				long min_frame = getHeaderFrameFromScreenPoint(event->pos().x() - CLICK_RANGE) - 1;
				long max_frame = getHeaderFrameFromScreenPoint(event->pos().x() + CLICK_RANGE) + 1;
				if (viewer->seq->workarea_in > min_frame && viewer->seq->workarea_in < max_frame) {
					resizing_workarea = true;
					resizing_workarea_in = true;
				} else if (viewer->seq->workarea_out > min_frame && viewer->seq->workarea_out < max_frame) {
					resizing_workarea = true;
					resizing_workarea_in = false;
				}
				if (resizing_workarea) {
					temp_workarea_in = viewer->seq->workarea_in;
					temp_workarea_out = viewer->seq->workarea_out;
					setCursor(Qt::SizeHorCursor);
				}
			}
		}
	}
}

void TimelineHeader::mouseReleaseEvent(QMouseEvent*) {
	if (viewer->seq != NULL) {
		dragging = false;
		if (resizing_workarea) {
			undo_stack.push(new SetTimelineInOutCommand(viewer->seq, true, temp_workarea_in, temp_workarea_out));
		} else if (dragging_markers && selected_markers.size() > 0) {
			bool moved = false;
			ComboAction* ca = new ComboAction();
			for (int i=0;i<selected_markers.size();i++) {
				Marker* m = &viewer->seq->markers[selected_markers.at(i)];
				if (selected_marker_original_times.at(i) != m->frame) {
					ca->append(new MoveMarkerAction(m, selected_marker_original_times.at(i), m->frame));
					moved = true;
				}
			}
			if (moved) {
				undo_stack.push(ca);
			} else {
				delete ca;
			}
		}

		resizing_workarea = false;
		dragging = false;
		dragging_markers = false;
		panel_timeline->snapped = false;
		update_parents();
	}
}

void TimelineHeader::focusOutEvent(QFocusEvent*) {
	selected_markers.clear();
	update();
}

void TimelineHeader::update_parents() {
	viewer->update_parents();
}

void TimelineHeader::update_zoom(double z) {
	zoom = z;
	update();
}

double TimelineHeader::get_zoom() {
	return zoom;
}

void TimelineHeader::delete_markers() {
	if (selected_markers.size() > 0) {
		DeleteMarkerAction* dma = new DeleteMarkerAction(viewer->seq);
		for (int i=0;i<selected_markers.size();i++) {
			dma->markers.append(selected_markers.at(i));
		}
		undo_stack.push(dma);
		update_parents();
	}
}

void TimelineHeader::paintEvent(QPaintEvent*) {
	if (viewer->seq != NULL && zoom > 0) {
		QPainter p(this);
		int yoff = (text_enabled) ? height()/2 : 0;

		double interval = viewer->seq->frame_rate;
		int textWidth = 0;
		int lastTextBoundary = INT_MIN;

		int i = 0;
		int lastLineX = INT_MIN;

		int sublineCount = 1;
		int sublineTest = qRound(interval*zoom);
		int sublineInterval = 1;
		while (sublineTest > SUBLINE_MIN_PADDING
			   && sublineInterval >= 1) {
			sublineCount *= 2;
			sublineInterval = (interval/sublineCount);
			sublineTest = qRound(sublineInterval*zoom);
		}
		sublineCount = qMin(sublineCount, qRound(interval));

		int text_x, fullTextWidth;
		QString timecode;

		while (true) {
			long frame = qRound(interval*i);
            int lineX = qRound(frame*zoom) - scroll;

			if (lineX > width()) break;

			// draw text
			bool draw_text = false;
			if (text_enabled && lineX-textWidth > lastTextBoundary) {
				timecode = frame_to_timecode(frame + in_visible, config.timecode_view, viewer->seq->frame_rate);
				fullTextWidth = fm.width(timecode);
				textWidth = fullTextWidth>>1;
				text_x = lineX-textWidth;
				lastTextBoundary = lineX+textWidth;
				if (lastTextBoundary >= 0) {
					draw_text = true;
				}
			}

			if (lineX > lastLineX+LINE_MIN_PADDING) {
				if (draw_text) {
					p.setPen(Qt::white);
					p.drawText(QRect(text_x, 0, fullTextWidth, yoff), timecode);
				}

				// draw line markers
				p.setPen(Qt::gray);
				p.drawLine(lineX, yoff, lineX, height());

				// draw sub-line markers
				for (int j=1;j<sublineCount;j++) {
					int sublineX = lineX+(qRound(j*interval/sublineCount)*zoom);
					p.drawLine(sublineX, yoff, sublineX, yoff+(height()/4));
				}

				lastLineX = lineX;
			}

			// TODO wastes cycles here, could just bring it up to 0
			i++;
		}

		// draw in/out selection
		int in_x;
		if (viewer->seq->using_workarea) {
			in_x = getHeaderScreenPointFromFrame((resizing_workarea ? temp_workarea_in : viewer->seq->workarea_in));
			int out_x = getHeaderScreenPointFromFrame((resizing_workarea ? temp_workarea_out : viewer->seq->workarea_out));
            p.fillRect(QRect(in_x, 0, out_x-in_x, height()), viewer->seq->enable_workarea ? QColor(0, 192, 255, 128) : QColor(255, 255, 255, 64));
			p.setPen(Qt::white);
			p.drawLine(in_x, 0, in_x, height());
			p.drawLine(out_x, 0, out_x, height());
		}

		// draw markers
		for (int i=0;i<viewer->seq->markers.size();i++) {
			const Marker& m = viewer->seq->markers.at(i);
			int marker_x = getHeaderScreenPointFromFrame(m.frame);
			const QPoint points[5] = {
				QPoint(marker_x, height()-1),
				QPoint(marker_x + MARKER_SIZE, height() - MARKER_SIZE - 1),
				QPoint(marker_x + MARKER_SIZE, yoff),
				QPoint(marker_x - MARKER_SIZE, yoff),
				QPoint(marker_x - MARKER_SIZE, height() - MARKER_SIZE - 1)
			};
			p.setPen(Qt::black);
			bool selected = false;
			for (int j=0;j<selected_markers.size();j++) {
				if (selected_markers.at(j) == i) {
					selected = true;
					break;
				}
			}
			if (selected) {
				p.setBrush(QColor(208, 255, 208));
			} else {
				p.setBrush(QColor(128, 224, 128));
			}
			p.drawPolygon(points, 5);
		}

		// draw playhead triangle
		p.setRenderHint(QPainter::Antialiasing);

		in_x = getHeaderScreenPointFromFrame(viewer->seq->playhead);
		QPoint start(in_x, height()+2);
		QPainterPath path;
		path.moveTo(start + QPoint(1,0));
		path.lineTo(in_x-PLAYHEAD_SIZE, yoff);
		path.lineTo(in_x+PLAYHEAD_SIZE+1, yoff);
		path.lineTo(start);
		p.fillPath(path, Qt::red);
	}
}

void TimelineHeader::show_context_menu(const QPoint &pos) {
	QMenu menu(this);

	mainWindow->make_inout_menu(&menu);

	menu.exec(mapToGlobal(pos));
}

void TimelineHeader::resized_scroll_listener(double d) {
	update_zoom(zoom * d);
}
