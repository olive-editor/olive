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

#include "keyframe.h"

#include <QVector>

#include "effectfield.h"
#include "undo.h"
#include "panels/panels.h"

EffectKeyframe::EffectKeyframe() {
	pre_handle_x = -40;
	pre_handle_y = 0;
	post_handle_x = 40;
	post_handle_y = 0;
}

void delete_keyframes(QVector<EffectField *>& selected_key_fields, QVector<int> &selected_keys) {
	QVector<EffectField*> fields;
	QVector<int> key_indices;

	for (int i=0;i<selected_keys.size();i++) {
		bool added = false;
		for (int j=0;j<key_indices.size();j++) {
			if (key_indices.at(j) < selected_keys.at(i)) {
				key_indices.insert(j, selected_keys.at(i));
				fields.insert(j, selected_key_fields.at(i));
				added = true;
				break;
			}
		}
		if (!added) {
			key_indices.append(selected_keys.at(i));
			fields.append(selected_key_fields.at(i));
		}
	}

	if (fields.size() > 0) {
		ComboAction* ca = new ComboAction();
		for (int i=0;i<key_indices.size();i++) {
			ca->append(new KeyframeDelete(fields.at(i), key_indices.at(i)));
		}
		Olive::UndoStack.push(ca);
		selected_keys.clear();
		selected_key_fields.clear();
		update_ui(false);
	}
}
