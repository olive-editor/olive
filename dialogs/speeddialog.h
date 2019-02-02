#ifndef SPEEDDIALOG_H
#define SPEEDDIALOG_H

#include <QDialog>

class Clip;
class LabelSlider;
class QCheckBox;

class SpeedDialog : public QDialog
{
	Q_OBJECT
public:
	SpeedDialog(QWidget* parent = 0);
	QVector<Clip*> clips;

	void run();
private slots:
	void percent_update();
	void duration_update();
	void frame_rate_update();
	void accept();
private:
	LabelSlider* percent;
	LabelSlider* duration;
	LabelSlider* frame_rate;

	QCheckBox* reverse;
	QCheckBox* maintain_pitch;
	QCheckBox* ripple;

	double default_frame_rate;
	double current_frame_rate;
	double current_percent;
	long default_length;
	long current_length;
};

#endif // SPEEDDIALOG_H
