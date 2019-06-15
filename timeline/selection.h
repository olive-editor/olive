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

#ifndef SELECTION_H
#define SELECTION_H

#include <QVector>

class Clip;
class Track;

class Selection {
public:
  Selection();
  Selection(long in, long out, Track* track);

  long in() const;
  long out() const;
  Track* track() const;

  void set_in(long in);
  void set_out(long out);

  bool ContainsTransition(Clip* c, int type) const;

  static void Tidy(QVector<Selection> &selections);

private:
  long in_;
  long out_;
  Track* track_;

  bool trim_in_;
};

#endif // SELECTION_H
