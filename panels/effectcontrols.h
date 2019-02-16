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

#ifndef EFFECTCONTROLS_H
#define EFFECTCONTROLS_H

#include <QDockWidget>
#include <QUndoCommand>
#include <QMutex>
#include <QMenu>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QLabel>
#include <QScrollBar>
#include <QHBoxLayout>

#include "project/projectelements.h"
#include "ui/timelineheader.h"
#include "ui/keyframeview.h"
#include "ui/resizablescrollbar.h"
#include "ui/keyframeview.h"

class EffectsArea : public QWidget {
	Q_OBJECT
public:
	EffectsArea(QWidget* parent = 0);
	QScrollArea* parent_widget;
	KeyframeView* keyframe_area;
	TimelineHeader* header;
public slots:
	void receive_wheel_event(QWheelEvent* e);
};

class EffectControls : public QDockWidget
{
	Q_OBJECT
public:
	explicit EffectControls(QWidget *parent = 0);
	~EffectControls();
    int get_mode();
	void set_clips(QVector<int>& clips, int mode);
	void clear_effects(bool clear_cache);
	void delete_effects();
	bool is_focused();
	void reload_clips();
	void set_zoom(bool in);
	bool keyframe_focus();
	void delete_selected_keyframes();
	bool multiple;
	void scroll_to_frame(long frame);

	QVector<int> selected_clips;

	double zoom;

	ResizableScrollBar* horizontalScrollBar;
	QScrollBar* verticalScrollBar;

	QMutex effects_loaded;

	void add_effect_paste_action(QMenu* menu);
public slots:
	void cut();
	void copy(bool del = false);
	void update_keyframes();
private slots:
	void menu_select(QAction* q);

	void video_effect_click();
	void audio_effect_click();
	void video_transition_click();
	void audio_transition_click();

	void deselect_all_effects(QWidget*);

	void update_scrollbar();
	void queue_post_update();

	void effects_area_context_menu();
protected:
	void resizeEvent(QResizeEvent *event);
private:
	void show_effect_menu(int type, int subtype);
	void load_effects();
	void load_keyframes();
	void open_effect(QVBoxLayout* hlayout, Effect* e);

	void setup_ui();

	int effect_menu_type;
	int effect_menu_subtype;
	QString panel_name;
	int mode;

	TimelineHeader* headers;
	EffectsArea* effects_area;
	QScrollArea* scrollArea;
	QLabel* lblMultipleClipsSelected;
	KeyframeView* keyframeView;
	QWidget* video_effect_area;
	QWidget* audio_effect_area;
	QWidget* vcontainer;
	QWidget* acontainer;
};

#endif // EFFECTCONTROLS_H
