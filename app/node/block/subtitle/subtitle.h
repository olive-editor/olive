/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2021 Olive Team

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

#ifndef SUBTITLEBLOCK_H
#define SUBTITLEBLOCK_H

#include "node/block/clip/clip.h"

namespace olive {

class SubtitleBlock : public ClipBlock
{
  Q_OBJECT
public:
  SubtitleBlock();

  NODE_DEFAULT_DESTRUCTOR(SubtitleBlock)

  virtual Node* copy() const override;

  virtual QString Name() const override;
  virtual QString id() const override;
  virtual QString Description() const override;

  virtual void Retranslate() override;

  static const QString kTextIn;

  QString GetText() const
  {
    return GetStandardValue(kTextIn).toString();
  }

  void SetText(const QString &text)
  {
    SetStandardValue(kTextIn, text);
  }

};

}

#endif // SUBTITLEBLOCK_H
