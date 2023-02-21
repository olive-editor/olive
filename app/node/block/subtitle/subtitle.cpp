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

#include "subtitle.h"

namespace olive {

#define super ClipBlock

const QString SubtitleBlock::kTextIn = QStringLiteral("text_in");

SubtitleBlock::SubtitleBlock()
{
  AddInput(kTextIn, NodeValue::kText, InputFlags(kInputFlagNotConnectable | kInputFlagNotKeyframable));

  SetInputFlag(kBufferIn, kInputFlagHidden);
  SetInputFlag(kLengthInput, kInputFlagHidden);
  SetInputFlag(kMediaInInput, kInputFlagHidden);
  SetInputFlag(kSpeedInput, kInputFlagHidden);
  SetInputFlag(kReverseInput, kInputFlagHidden);
  SetInputFlag(kMaintainAudioPitchInput, kInputFlagHidden);

  // Undo block flag that hides in param view
  SetFlag(kDontShowInParamView, false);
}

QString SubtitleBlock::Name() const
{
  if (GetText().isEmpty()) {
    return tr("Subtitle");
  } else {
    return GetText();
  }
}

QString SubtitleBlock::id() const
{
  return QStringLiteral("org.olivevideoeditor.Olive.subtitle");
}

QString SubtitleBlock::Description() const
{
  return tr("A time-based node representing a single subtitle element for a certain period of time.");
}

void SubtitleBlock::Retranslate()
{
  super::Retranslate();

  SetInputName(kTextIn, tr("Text"));
}

}
