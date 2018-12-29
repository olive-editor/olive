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
    void copy(bool del);
	bool multiple;

	QVector<int> selected_clips;

	double zoom;

	Ui::EffectControls *ui;
public slots:
    void update_keyframes();
private slots:
    void menu_select(QAction* q);

    void on_btnAddVideoEffect_clicked();
    void on_btnAddAudioEffect_clicked();
    void on_btnAddVideoTransition_clicked();
    void on_btnAddAudioTransition_clicked();

    void deselect_all_effects(QWidget*);
protected:
	void resizeEvent(QResizeEvent *event);
private:
    void show_effect_menu(int type, int subtype);
	void load_effects();
	void load_keyframes();
    void open_effect(QVBoxLayout* layout, Effect* e);

    int effect_menu_type;
    int effect_menu_subtype;
    QString panel_name;
    int mode;
};

#endif // EFFECTCONTROLS_H
