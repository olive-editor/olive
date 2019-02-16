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

#include "clipboard.h"

#include "project/clip.h"
#include "project/effect.h"
#include "project/transition.h"

int clipboard_type = CLIPBOARD_TYPE_CLIP;
QVector<void*> clipboard;
QVector<Transition*> clipboard_transitions;

void clear_clipboard() {
	int clipboard_size = clipboard.size();
	for (int i=0;i<clipboard_size;i++) {
		if (clipboard_type == CLIPBOARD_TYPE_CLIP) {
			delete static_cast<Clip*>(clipboard.at(i));
		} else if (clipboard_type == CLIPBOARD_TYPE_EFFECT) {
			delete static_cast<Effect*>(clipboard.at(i));
		}
	}
	clipboard_size = clipboard_transitions.size();
	for (int i=0;i<clipboard_size;i++) {
		delete clipboard_transitions.at(i);
	}
	clipboard.clear();
}
