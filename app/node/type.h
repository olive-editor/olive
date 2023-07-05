/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2023 Olive Studios LLC

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

#ifndef TYPE_H
#define TYPE_H

#include <QString>

namespace olive {

class type_t
{
  public:
  constexpr type_t() : type_(0) {}
  constexpr type_t(const char *x) : type_(insn_to_num(x)) {}

  static type_t fromString(const QStringView &s) { return s.toUtf8().constData(); }
  QString toString() const
  {
    const char *c = reinterpret_cast<const char*>(&type_);
    return QString::fromUtf8(c, strnlen(c, sizeof(type_)));
  }

  bool operator==(const type_t &t) const { return type_ == t.type_; }
  bool operator!=(const type_t &t) const { return !(*this == t); }
  bool operator<(const type_t &t) const { return type_ < t.type_; }
  bool operator<=(const type_t &t) const { return type_ <= t.type_; }
  bool operator>(const type_t &t) const { return type_ > t.type_; }
  bool operator>=(const type_t &t) const { return type_ >= t.type_; }

  private:
  constexpr uint64_t insn_to_num(const char* x)
  {
    return (x && *x) ? *x + (insn_to_num(x+1) << 8) : 0;
  }

  uint64_t type_;

};

}

#endif // TYPE_H
