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

#ifndef CLIPBOARD_H
#define CLIPBOARD_H

#include <QVector>

#include <effects/transition.h>

#define CLIPBOARD_TYPE_CLIP 0
#define CLIPBOARD_TYPE_EFFECT 1

using VoidPtr = std::shared_ptr<void>;

extern int clipboard_type;
extern QVector<TransitionPtr> clipboard_transitions;
extern QVector<VoidPtr> clipboard;
void clear_clipboard();

#endif // CLIPBOARD_H
