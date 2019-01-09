/* 
 * Olive. Olive is a free non-linear video editor for Windows, macOS, and Linux.
 * Copyright (C) 2018  {{ organization }}
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 *along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef LABELSLIDER_H
#define LABELSLIDER_H

#include <QLabel>
#include <QUndoCommand>

#define LABELSLIDER_NORMAL 0
#define LABELSLIDER_FRAMENUMBER 1
#define LABELSLIDER_PERCENT 2

class LabelSlider : public QLabel
{
    Q_OBJECT
public:
    LabelSlider(QWidget* parent = 0);
	void set_frame_rate(double d);
	void set_display_type(int type);
	void set_value(double v, bool userSet);
    void set_default_value(double v);
    void set_minimum_value(double v);
    void set_maximum_value(double v);
    double value();
    bool is_set();
	bool is_dragging();
	QString valueToString(double v);
	double getPreviousValue();
    void set_previous_value();
	void set_color(QString c = 0);
	int decimal_places;
protected:
    void mousePressEvent(QMouseEvent *ev);
    void mouseMoveEvent(QMouseEvent *ev);
    void mouseReleaseEvent(QMouseEvent *ev);
private:
    double default_value;
    double internal_value;
    double drag_start_value;
	double previous_value;

    bool min_enabled;
    double min_value;
    bool max_enabled;
    double max_value;

    bool drag_start;
    bool drag_proc;
    int drag_start_x;
    int drag_start_y;

    bool set;

	int display_type;

	double frame_rate;
signals:
	void valueChanged();
	void clicked();
};

#endif // LABELSLIDER_H
