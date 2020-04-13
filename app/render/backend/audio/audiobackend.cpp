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

#include "audiobackend.h"

#include "audioworker.h"

OLIVE_NAMESPACE_ENTER

AudioBackend::AudioBackend(QObject *parent) :
  AudioRenderBackend(parent)
{
}

AudioBackend::~AudioBackend()
{
  Close();
}

QIODevice *AudioBackend::GetAudioPullDevice()
{
  pull_device_.setFileName(CachePathName());

  return &pull_device_;
}

bool AudioBackend::InitInternal()
{
  // Initiate one thread per CPU core
  for (int i=0;i<threads().size();i++) {
    // Create one processor object for each thread
    AudioWorker* processor = new AudioWorker(&copy_map_);
    processor->SetParameters(params());
    processors_.append(processor);
  }

  return true;
}

void AudioBackend::CloseInternal()
{
}

bool AudioBackend::CompileInternal()
{
  // This backend doesn't compile anything yet
  return AudioRenderBackend::CompileInternal();
}

void AudioBackend::DecompileInternal()
{
  // This backend doesn't compile anything yet
  AudioRenderBackend::DecompileInternal();
}

void AudioBackend::ConnectWorkerToThis(RenderWorker *worker)
{
  AudioRenderBackend::ConnectWorkerToThis(worker);

  connect(worker, &RenderWorker::CompletedCache, this, &AudioBackend::ThreadCompletedCache);
}

void AudioBackend::ThreadCompletedCache(NodeDependency dep, NodeValueTable data, qint64 job_time)
{
  SetWorkerBusyState(static_cast<RenderWorker*>(sender()), false);

  if (job_time == render_job_info_.value(dep.range())) {
    render_job_info_.remove(dep.range());

    QByteArray cached_samples = data.Get(NodeParam::kSamples).value<SampleBufferPtr>()->toPackedData();

    int offset = params().time_to_bytes(dep.in());
    int length = params().time_to_bytes(dep.range().length());
    int out_point = qMin(offset + length, params().time_to_bytes(GetSequenceLength()));

    if (offset < out_point) {
      if (offset + length > out_point) {
        length = out_point - offset;
      }

      QFile f(CachePathName());
      if (f.open(QFile::ReadWrite)) {

        if (f.size() < out_point && !f.resize(out_point)) {
          qCritical() << "Failed to resize file" << CachePathName();
        }

        if (!f.seek(offset)) {
          qCritical() << "Failed to seek file" << CachePathName();
        }

        // Replace data with this data
        int copy_length = qMin(length, cached_samples.size());

        f.write(cached_samples.data(), copy_length);

        if (copy_length < length) {

          // Fill in remainder with silence
          QByteArray empty_space(length - copy_length, 0);
          f.write(empty_space);
        }

        f.close();
      } else {
        qWarning() << "Failed to write to cached PCM file";
      }
    }
  }

  CacheNext();
}

OLIVE_NAMESPACE_EXIT
