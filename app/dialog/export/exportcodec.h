/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2019 Olive Team

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

#ifndef EXPORTCODEC_H
#define EXPORTCODEC_H

#include <QString>

#include "common/define.h"

OLIVE_NAMESPACE_ENTER

class ExportCodec {
public:
  enum Flag {
    kNone,
    kStillImage
  };

  ExportCodec(const QString& name, const QString& id, Flag flags = kNone) :
    name_(name),
    id_(id),
    flags_(flags)
  {
  }

  const QString& name() const {return name_;}
  const QString& id() const {return id_;}
  const Flag& flags() const {return flags_;}

private:
  QString name_;
  QString id_;
  Flag flags_;

};

OLIVE_NAMESPACE_EXIT

#endif // EXPORTCODEC_H
