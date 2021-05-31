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

#include "conform.h"

namespace olive {

ConformTask::ConformTask(const QString &decoder_id, const Decoder::CodecStream &stream, const AudioParams& params, const QString &output_filename) :
  decoder_id_(decoder_id),
  stream_(stream),
  params_(params),
  output_filename_(output_filename)
{
  SetTitle(tr("Conforming Audio %1:%2").arg(stream.filename(), QString::number(stream.stream())));
}

bool ConformTask::Run()
{
  DecoderPtr decoder = Decoder::CreateFromID(decoder_id_);

  if (!decoder->Open(stream_)) {
    SetError(tr("Failed to open decoder for audio conform"));
    return false;
  }

  connect(decoder.get(), &Decoder::IndexProgress, this, &ConformTask::ProgressChanged);

  bool ret = decoder->ConformAudio(output_filename_, params_, &IsCancelled());

  decoder->Close();

  return ret;
}

}
