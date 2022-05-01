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

#include "planarprocessor.h"

#include "common/ffmpegutils.h"

namespace olive {

PlanarProcessor::PlanarProcessor() :
  swr_ctx_(nullptr)
{
}

PlanarProcessor::~PlanarProcessor()
{
  Close();
}

bool PlanarProcessor::Open(const AudioParams &params)
{
  if (IsOpen()) {
    return true;
  }

  swr_ctx_ = swr_alloc_set_opts(nullptr,
                                params.channel_layout(),
                                FFmpegUtils::GetFFmpegSampleFormat(params.format(), true),
                                params.sample_rate(),
                                params.channel_layout(),
                                FFmpegUtils::GetFFmpegSampleFormat(params.format(), false),
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

  params_ = params;

  return true;
}

SampleBufferPtr PlanarProcessor::Convert(const QByteArray &packed)
{
  if (!IsOpen()) {
    qCritical() << "Tried to convert while closed";
    return nullptr;
  }

  if (packed.isEmpty()) {
    return nullptr;
  }

  int nb_samples_per_channel = params_.bytes_to_samples(packed.size());

  SampleBufferPtr output = SampleBuffer::CreateAllocated(params_, nb_samples_per_channel);

  const uint8_t *input = reinterpret_cast<const uint8_t*>(packed.constData());
  int ret = swr_convert(swr_ctx_,
                        reinterpret_cast<uint8_t**>(output->to_raw_ptrs()), nb_samples_per_channel,
                        &input, nb_samples_per_channel);
  if (ret < 0) {
    char buf[200];
    av_strerror(ret, buf, 200);
    qDebug() << "Planar processor failed with error:" << buf << ret;
  }

  return output;
}

void PlanarProcessor::Close()
{
  if (swr_ctx_) {
    swr_free(&swr_ctx_);
  }
}

}
