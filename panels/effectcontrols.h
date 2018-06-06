#ifndef EFFECTCONTROLS_H
#define EFFECTCONTROLS_H

#include <QDockWidget>

struct Clip;
class QMenu;

namespace Ui {
class EffectControls;
}

class EffectControls : public QDockWidget
{
	Q_OBJECT

public:
	explicit EffectControls(QWidget *parent = 0);
	~EffectControls();
	void set_clip(Clip* c);

private slots:
	void on_pushButton_clicked();
    void menu_select(QAction* q);

private:
	Ui::EffectControls *ui;
	Clip* clip;
    QMenu* effects_menu;
};

#endif // EFFECTCONTROLS_H
