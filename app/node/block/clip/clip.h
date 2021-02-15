/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2020 Olive Team

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

#ifndef CLIPBLOCK_H
#define CLIPBLOCK_H

#include "node/block/block.h"

namespace olive {

/**
 * @brief Node that represents a block of Media
 */
class ClipBlock : public Block
{
  Q_OBJECT
public:
  ClipBlock();

  virtual Node* copy() const override;

  virtual Type type() const override;

  virtual QString Name() const override;
  virtual QString id() const override;
  virtual QString Description() const override;

  virtual void InvalidateCache(const TimeRange& range, const QString& from, int element = -1) override;

  virtual TimeRange InputTimeAdjustment(const QString& input, int element, const TimeRange& input_time) const override;

  virtual TimeRange OutputTimeAdjustment(const QString& input, int element, const TimeRange& input_time) const override;

  virtual NodeValueTable Value(const QString& output, NodeValueDatabase& value) const override;

  virtual void Retranslate() override;

  virtual void Hash(const QString& output, QCryptographicHash &hash, const rational &time) const override;

  static const QString kBufferIn;

};

}

#endif // TIMELINEBLOCK_H
