/***

    Olive - Non-Linear Video Editor
    Copyright (C) 2019  Olive Team

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

***/

#ifndef AUDIOMONITOR_H
#define AUDIOMONITOR_H

#include <QWidget>
#include <QTimer>

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
	QTimer clear_timer;

private slots:
	void clear();
};

#endif // AUDIOMONITOR_H
