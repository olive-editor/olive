#ifndef EFFECTCONTROLS_H
#define EFFECTCONTROLS_H

#include <QDockWidget>
#include <QUndoCommand>

struct Clip;
class QMenu;
class Effect;
class TimelineHeader;
class QScrollArea;
class KeyframeView;
class QVBoxLayout;

class EffectsArea : public QWidget {
public:
	EffectsArea(QWidget* parent = 0);
	void resizeEvent(QResizeEvent *event);
	QScrollArea* parent_widget;
	KeyframeView* keyframe_area;
	TimelineHeader* header;
};

namespace Ui {
class EffectControls;
}

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
	bool multiple;

	QVector<int> selected_clips;

	double zoom;

	Ui::EffectControls *ui;
public slots:
    void update_keyframes();
private slots:
    void menu_select(QAction* q);
    void on_add_video_effect_button_clicked();
    void on_add_audio_effect_button_clicked();
	void deselect_all_effects(QWidget*);

	void on_add_video_transition_button_clicked();
	void on_add_audio_transition_button_clicked();
protected:
	void resizeEvent(QResizeEvent *event);
private:
	void show_effect_menu(bool video, bool transitions);
	void load_effects();
	void load_keyframes();
    void open_effect(QVBoxLayout* layout, Effect* e);

	bool video_menu;
	bool transition_menu;
    QString panel_name;
    int mode;
};

#endif // EFFECTCONTROLS_H
