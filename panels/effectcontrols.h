#ifndef EFFECTCONTROLS_H
#define EFFECTCONTROLS_H

#include <QDockWidget>
#include <QUndoCommand>

struct Clip;
class QMenu;
class Effect;

namespace Ui {
class EffectControls;
}

class EffectControls : public QDockWidget
{
	Q_OBJECT

public:
	explicit EffectControls(QWidget *parent = 0);
	~EffectControls();
	// void set_clip(Clip* c);
    void set_clips(QVector<int>& clips);
    void clear_effects(bool clear_cache);
    void delete_effects();
    bool is_focused();
    void reload_clips();

private slots:
    void menu_select(QAction* q);

    void on_add_video_effect_button_clicked();

    void on_add_audio_effect_button_clicked();

    void deselect_all_effects(QWidget*);

private:
    Ui::EffectControls *ui;
    QVector<int> selected_clips;
    void show_menu(bool video);
	void load_effects();

	bool video_menu;
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
