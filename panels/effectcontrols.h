#ifndef EFFECTCONTROLS_H
#define EFFECTCONTROLS_H

#include <QDockWidget>
#include <QUndoCommand>
#include <QMutex>

struct Clip;
class QMenu;
class Effect;
class TimelineHeader;
class QScrollArea;
class KeyframeView;
class QVBoxLayout;
class ResizableScrollBar;
class QLabel;
class KeyframeView;
class QScrollBar;

class EffectsArea : public QWidget {
public:
	EffectsArea(QWidget* parent = 0);
	void resizeEvent(QResizeEvent *event);
	QScrollArea* parent_widget;
	KeyframeView* keyframe_area;
	TimelineHeader* header;
};

class EffectControls : public QDockWidget
{
	Q_OBJECT

public:
	explicit EffectControls(QWidget *parent = 0);
	~EffectControls();
	void set_clips(QVector<int>& clips, int mode);
	void clear_effects(bool clear_cache);
	void delete_effects();
	bool is_focused();
	void reload_clips();
	void set_zoom(bool in);
	bool keyframe_focus();
	void delete_selected_keyframes();
	void copy(bool del);
	bool multiple;
	void scroll_to_frame(long frame);

	QVector<int> selected_clips;

	double zoom;

	ResizableScrollBar* horizontalScrollBar;
	QScrollBar* verticalScrollBar;

    QMutex effects_loaded;
public slots:
	void update_keyframes();
private slots:
	void menu_select(QAction* q);

	void video_effect_click();
	void audio_effect_click();
	void video_transition_click();
	void audio_transition_click();

	void deselect_all_effects(QWidget*);
protected:
	void resizeEvent(QResizeEvent *event);
private:
	void show_effect_menu(int type, int subtype);
	void load_effects();
	void load_keyframes();
	void open_effect(QVBoxLayout* layout, Effect* e);

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
