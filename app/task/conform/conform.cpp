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

#include "conform.h"

#include "codec/decoder.h"

namespace olive {

ConformTask::ConformTask(Footage* footage, int index, const AudioParams& params) :
  footage_(footage),
  index_(index),
  params_(params)
{
  SetTitle(tr("Conforming Audio %1:%2").arg(footage_->filename(), QString::number(index_)));
}

bool ConformTask::Run()
{
  // Conforming is done by the renderer now, but I would like to use something like this just to
  // show progress

  /*if (stream_->footage()->decoder().isEmpty()) {
    SetError(tr("Failed to find decoder to conform audio stream"));
    return false;
  } else {
    DecoderPtr decoder = Decoder::CreateFromID(stream_->footage()->decoder());

    decoder->set_stream(stream_);

    connect(decoder.get(), &Decoder::IndexProgress, this, &ConformTask::ProgressChanged);

    if (!decoder->ConformAudio(&IsCancelled(), params_)) {
      SetError(tr("Failed to conform audio"));
      return false;
    } else {
      return true;
    }
  }*/

  return true;
}

}
