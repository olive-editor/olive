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

#ifndef CLIPBLOCK_H
#define CLIPBLOCK_H

#include "node/block/block.h"

OLIVE_NAMESPACE_ENTER

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

  NodeInput* texture_input() const;

  virtual void InvalidateCache(const rational &start_range, const rational &end_range, NodeInput *from = nullptr) override;

  virtual TimeRange InputTimeAdjustment(NodeInput* input, const TimeRange& input_time) const override;

  virtual TimeRange OutputTimeAdjustment(NodeInput* input, const TimeRange& input_time) const override;

  virtual NodeValueTable Value(const NodeValueDatabase& value) const override;

  virtual void Retranslate() override;

signals:
  void PreviewUpdated();

private:
  NodeInput* texture_input_;

};

OLIVE_NAMESPACE_EXIT

#endif // TIMELINEBLOCK_H
