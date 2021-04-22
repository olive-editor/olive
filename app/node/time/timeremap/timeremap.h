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

#ifndef TIMEREMAPNODE_H
#define TIMEREMAPNODE_H

#include "node/node.h"

namespace olive {

class TimeRemapNode : public Node
{
  Q_OBJECT
public:
  TimeRemapNode();

  NODE_DEFAULT_DESTRUCTOR(TimeRemapNode)

  virtual Node* copy() const override;

  virtual QString Name() const override;
  virtual QString id() const override;
  virtual QVector<CategoryID> Category() const override;
  virtual QString Description() const override;

  virtual TimeRange InputTimeAdjustment(const QString& input, int element, const TimeRange& input_time) const override;
  virtual TimeRange OutputTimeAdjustment(const QString& input, int element, const TimeRange& input_time) const override;

  virtual void Retranslate() override;

  virtual QVector<QString> inputs_for_output(const QString &output) const override;

  virtual void Hash(const QString &output, QCryptographicHash &hash, const rational &time, const VideoParams& video_params) const override;

  static const QString kTimeInput;
  static const QString kInputInput;

private:
  rational GetRemappedTime(const rational& input) const;

};

}

#endif // TIMEREMAPNODE_H
