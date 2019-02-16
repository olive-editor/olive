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

#ifndef KEYFRAME_H
#define KEYFRAME_H

#include <QVariant>

class EffectField;

class EffectKeyframe {
public:
	EffectKeyframe();

	long time;
	int type;
	QVariant data;

	// only for bezier type
	double pre_handle_x;
	double pre_handle_y;
	double post_handle_x;
	double post_handle_y;
};

void delete_keyframes(QVector<EffectField *> &selected_key_fields, QVector<int> &selected_keys);

#endif // KEYFRAME_H
