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

#include "packedprocessor.h"

#include "common/ffmpegutils.h"

namespace olive {

PackedProcessor::PackedProcessor() :
  swr_ctx_(nullptr)
{
}

PackedProcessor::~PackedProcessor()
{
  Close();
}

bool PackedProcessor::Open(const AudioParams &params)
{
  if (IsOpen()) {
    return true;
  }

  swr_ctx_ = swr_alloc_set_opts(nullptr,
                                params.channel_layout(),
                                FFmpegUtils::GetFFmpegSampleFormat(params.format(), false),
                                params.sample_rate(),
                                params.channel_layout(),
                                FFmpegUtils::GetFFmpegSampleFormat(params.format(), true),
                                params.sample_rate(),
                                0,
                                nullptr);

  if (!swr_ctx_) {
    qCritical() << "Failed to allocate resample context";
    return false;
  }

  if (swr_init(swr_ctx_) < 0) {
    qCritical() << "Failed to init resample context";
    swr_free(&swr_ctx_);
    return false;
  }

  return true;
}

QByteArray PackedProcessor::Convert(SampleBufferPtr planar)
{
  if (!IsOpen()) {
    qCritical() << "Tried to convert while closed";
    return QByteArray();
  }

  int nb_samples = planar->sample_count();
  if (nb_samples == 0) {
    return QByteArray();
  }

  QByteArray output(planar->audio_params().samples_to_bytes(nb_samples), Qt::Uninitialized);
  uint8_t *output_data = reinterpret_cast<uint8_t*>(output.data());

  int ret = swr_convert(swr_ctx_, &output_data, nb_samples,
                        const_cast<const uint8_t**>(reinterpret_cast<uint8_t**>(planar->to_raw_ptrs())),
                        nb_samples);
  if (ret < 0) {
    char buf[200];
    av_strerror(ret, buf, 200);
    qDebug() << "Packed processor failed with error:" << buf << ret;
  }

  return output;
}

void PackedProcessor::Close()
{
  if (swr_ctx_) {
    swr_free(&swr_ctx_);
  }
}

}
