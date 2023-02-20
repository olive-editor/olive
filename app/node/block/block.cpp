/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2022 Olive Team

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

#include "block.h"

#include <QDebug>

#include "node/inputdragger.h"
#include "node/output/track/track.h"
#include "transition/transition.h"
#include "widget/slider/rationalslider.h"

namespace olive {

#define super Node

const QString Block::kLengthInput = QStringLiteral("length_in");

Block::Block() :
  previous_(nullptr),
  next_(nullptr),
  track_(nullptr),
  index_(-1)
{
  AddInput(kLengthInput, NodeValue::kRational, InputFlags(kInputFlagNotConnectable | kInputFlagNotKeyframable | kInputFlagHidden));
  SetInputProperty(kLengthInput, QStringLiteral("min"), QVariant::fromValue(rational(0, 1)));
  SetInputProperty(kLengthInput, QStringLiteral("view"), RationalSlider::kTime);
  SetInputProperty(kLengthInput, QStringLiteral("viewlock"), true);

  SetInputFlags(kEnabledInput, InputFlags(GetInputFlags(kEnabledInput) | kInputFlagNotConnectable | kInputFlagNotKeyframable));

  SetFlags(kDontShowInParamView);
}

QVector<Node::CategoryID> Block::Category() const
{
  return {kCategoryTimeline};
}

rational Block::length() const
{
  return GetStandardValue(kLengthInput).value<rational>();
}

void Block::set_length_and_media_out(const rational &length)
{
  if (length == this->length()) {
    return;
  }

  set_length_internal(length);
}

void Block::set_length_and_media_in(const rational &length)
{
  if (length == this->length()) {
    return;
  }

  // Set the length without setting media out
  set_length_internal(length);
}

bool Block::is_enabled() const
{
  return GetStandardValue(kEnabledInput).toBool();
}

void Block::set_enabled(bool e)
{
  SetStandardValue(kEnabledInput, e);

  emit EnabledChanged();
}

void Block::InputValueChangedEvent(const QString &input, int element)
{
  super::InputValueChangedEvent(input, element);

  if (input == kLengthInput) {
    emit LengthChanged();
  } else if (input == kEnabledInput) {
    emit EnabledChanged();
  }
}

void Block::set_length_internal(const rational &length)
{
  SetStandardValue(kLengthInput, QVariant::fromValue(length));
}

void Block::Retranslate()
{
  super::Retranslate();

  SetInputName(kLengthInput, tr("Length"));
  SetInputName(kEnabledInput, tr("Enabled"));
}

void Block::InvalidateCache(const TimeRange& range, const QString& from, int element, InvalidateCacheOptions options)
{
  TimeRange r;

  if (from == kLengthInput) {
    // We must intercept the signal here
    r = TimeRange(qMin(length(), last_length_), RATIONAL_MAX);

    if (!NodeInputDragger::IsInputBeingDragged()) {
      last_length_ = length();
    }

    options.insert(QStringLiteral("lengthevent"), true);
  } else {
    r = range;
  }

  super::InvalidateCache(r, from, element, options);
}

void Block::set_previous_next(Block *previous, Block *next)
{
  if (previous) {
    previous->set_next(next);
  }
  if (next) {
    next->set_previous(previous);
  }
}

}
