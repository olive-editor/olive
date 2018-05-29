#ifndef EFFECTCONTROLS_H
#define EFFECTCONTROLS_H

#include <QDockWidget>

struct Clip;

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

private:
	Ui::EffectControls *ui;
	Clip* clip;
};

#endif // EFFECTCONTROLS_H
