#ifndef SPEEDDIALOG_H
#define SPEEDDIALOG_H

#include <QDialog>

struct Clip;
class LabelSlider;
class QCheckBox;

class SpeedDialog : public QDialog
{
public:
	SpeedDialog(QWidget* parent = 0);
	QVector<Clip*> clips;
private:
	LabelSlider* percent;
	LabelSlider* duration;
	LabelSlider* frame_rate;

	QCheckBox* reverse;
};

#endif // SPEEDDIALOG_H
