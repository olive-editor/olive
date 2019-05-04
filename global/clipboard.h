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

#include "effects/transition.h"
#include "project/media.h"

using VoidPtr = std::shared_ptr<void>;

class Clipboard {
public:
  enum Type {
    CLIPBOARD_TYPE_CLIP,
    CLIPBOARD_TYPE_EFFECT
  };

  Clipboard();
  void Append(VoidPtr obj);
  void Insert(int pos, VoidPtr obj);
  void RemoveAt(int pos);
  void Clear();
  int Count();
  VoidPtr Get(int i);
  void SetType(Type type);
  bool IsEmpty();
  Type type();

  bool DeleteClipsWithMedia(ComboAction* ca, Media* m);

private:
  Type type_;

  QVector<TransitionPtr> clipboard_transitions_;
  QVector<VoidPtr> clipboard_;
};

namespace olive {
extern Clipboard clipboard;
}

#endif // CLIPBOARD_H
