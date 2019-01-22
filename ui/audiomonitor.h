#ifndef AUDIOMONITOR_H
#define AUDIOMONITOR_H

#include <QWidget>

class AudioMonitor : public QWidget
{
	Q_OBJECT
public:
	explicit AudioMonitor(QWidget *parent = 0);
	void set_value(const QVector<double>& values);

protected:
	void paintEvent(QPaintEvent *);
	void resizeEvent(QResizeEvent *);

signals:

public slots:

private:
	QLinearGradient gradient;
	QVector<double> values;
};

#endif // AUDIOMONITOR_H
