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
    void set_clips(QVector<int>& clips);
    void clear_effects(bool clear_cache);
    void delete_effects();
    bool is_focused();
    void reload_clips();
	void update_keyframes();
	void set_zoom(bool in);
	bool keyframe_focus();

	double zoom;
private slots:
    void menu_select(QAction* q);
    void on_add_video_effect_button_clicked();
    void on_add_audio_effect_button_clicked();
	void deselect_all_effects(QWidget*);

	void on_add_video_transition_button_clicked();

	void on_add_audio_transition_button_clicked();

private:
	Ui::EffectControls *ui;
    QVector<int> selected_clips;
	void show_effect_menu(bool video, bool transitions);
	void load_effects();
	void load_keyframes();

	bool video_menu;
	bool transition_menu;
};

/*class EffectAddCommand : public QUndoCommand {
public:
    EffectAddCommand();
    ~EffectAddCommand();
    void undo();
    void redo();
    QVector<Clip*> clips;
    QVector<Effect*> effects;
private:
    bool done;
};*/

#endif // EFFECTCONTROLS_H
