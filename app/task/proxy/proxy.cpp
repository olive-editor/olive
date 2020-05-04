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

#include "proxy.h"

#include "codec/decoder.h"

OLIVE_NAMESPACE_ENTER

ProxyTask::ProxyTask(VideoStreamPtr stream, int divider) :
  stream_(stream),
  divider_(divider)
{
  if (divider_ == 1) {
    SetTitle(tr("Generating full resolution proxy %1:%2").arg(stream_->footage()->filename(),
                                                              QString::number(stream_->index())));
  } else {
    SetTitle(tr("Generating 1/%1 resolution proxy %2:%3").arg(QString::number(divider),
                                                              stream_->footage()->filename(),
                                                              QString::number(stream_->index())));
  }
}

void ProxyTask::Action()
{
  if (stream_->footage()->decoder().isEmpty()) {
    emit Failed(tr("Failed to find decoder to conform audio stream"));
  } else {
    DecoderPtr decoder = Decoder::CreateFromID(stream_->footage()->decoder());

    decoder->set_stream(stream_);

    connect(decoder.get(), &Decoder::IndexProgress, this, &ProxyTask::ProgressChanged);

    if (decoder->ProxyVideo(&IsCancelled(), divider_)) {
      emit Succeeded();
    } else {
      emit Failed(QStringLiteral("Failed to generate proxy"));
    }
  }
}

OLIVE_NAMESPACE_EXIT
