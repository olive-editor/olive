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

#ifndef KEYFRAMENAVIGATOR_H
#define KEYFRAMENAVIGATOR_H

#include <QWidget>

class QHBoxLayout;
class QPushButton;

class KeyframeNavigator : public QWidget
{
	Q_OBJECT
public:
	KeyframeNavigator(QWidget* parent = 0, bool addLeftPad = true);
	~KeyframeNavigator();
	void enable_keyframes(bool);
	void enable_keyframe_toggle(bool);
signals:
	void goto_previous_key();
	void toggle_key();
	void goto_next_key();
	void keyframe_enabled_changed(bool);
	void clicked();
private slots:
	void keyframe_ui_enabled(bool);
private:
	QHBoxLayout* key_controls;
	QPushButton* left_key_nav;
	QPushButton* key_addremove;
	QPushButton* right_key_nav;
	QPushButton* keyframe_enable;
};

#endif // KEYFRAMENAVIGATOR_H
