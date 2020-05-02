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

#include "decoder.h"

#include <QCoreApplication>
#include <QDebug>
#include <QFileInfo>

#include "codec/ffmpeg/ffmpegcommon.h"
#include "codec/ffmpeg/ffmpegdecoder.h"
#include "codec/oiio/oiiodecoder.h"
#include "codec/waveinput.h"
#include "codec/waveoutput.h"
#include "render/backend/indexmanager.h"
#include "task/index/index.h"
#include "task/taskmanager.h"

OLIVE_NAMESPACE_ENTER

Decoder::Decoder() :
  open_(false),
  stream_(nullptr)
{
}

Decoder::Decoder(Stream *fs) :
  open_(false),
  stream_(fs)
{
}

StreamPtr Decoder::stream()
{
  return stream_;
}

void Decoder::set_stream(StreamPtr fs)
{
  Close();

  stream_ = fs;
}

FramePtr Decoder::RetrieveVideo(const rational &/*timecode*/, const int &/*divider*/)
{
  return nullptr;
}

SampleBufferPtr Decoder::RetrieveAudio(const rational &/*timecode*/, const rational &/*length*/, const AudioRenderingParams &/*params*/)
{
  return nullptr;
}

bool Decoder::SupportsVideo()
{
  return false;
}

bool Decoder::SupportsAudio()
{
  return false;
}

/*
 * DECODER STATIC PUBLIC MEMBERS
 */

QVector<DecoderPtr> ReceiveListOfAllDecoders() {
  QVector<DecoderPtr> decoders;

  // The order in which these decoders are added is their priority when probing. Hence FFmpeg should usually be last,
  // since it supports so many formats and we presumably want to override those formats with a more specific decoder.

  decoders.append(std::make_shared<OIIODecoder>());
  decoders.append(std::make_shared<FFmpegDecoder>());

  return decoders;
}

bool Decoder::ProbeMedia(Footage *f, const QAtomicInt* cancelled)
{
  // Check for a valid filename
  if (f->filename().isEmpty()) {
    qWarning() << "Tried to probe media with an empty filename";
    return false;
  }

  // Check file exists
  if (!QFileInfo::exists(f->filename())) {
    qWarning() << "Tried to probe file that doesn't exist:" << f->filename();
    return false;
  }

  // Reset Footage state for probing
  f->Clear();

  // Create list to iterate through
  QVector<DecoderPtr> decoder_list = ReceiveListOfAllDecoders();

  // Pass Footage through each Decoder's probe function
  for (int i=0;i<decoder_list.size();i++) {

    if (cancelled && *cancelled) {
      return false;
    }

    DecoderPtr decoder = decoder_list.at(i);

    if (decoder->Probe(f, cancelled)) {

      // We found a Decoder, so we can set this media as valid
      f->set_status(Footage::kReady);

      // Attach the successful Decoder to this Footage object
      f->set_decoder(decoder->id());

      // FIXME: Cache the results so we don't have to probe if this media is added a second time

      // Start an index task
      foreach (StreamPtr stream, f->streams()) {
        if (stream->type() == Stream::kAudio) {
          QMetaObject::invokeMethod(IndexManager::instance(),
                                    "StartIndexingStream",
                                    Qt::QueuedConnection,
                                    OLIVE_NS_ARG(StreamPtr, stream));
        }
      }

      return true;
    }
  }

  // We aren't able to use this Footage
  f->set_status(Footage::kInvalid);
  f->set_decoder(QString());

  return false;
}

DecoderPtr Decoder::CreateFromID(const QString &id)
{
  if (id.isEmpty()) {
    return nullptr;
  }

  // Create list to iterate through
  QVector<DecoderPtr> decoder_list = ReceiveListOfAllDecoders();

  foreach (DecoderPtr d, decoder_list) {
    if (d->id() == id) {
      return d;
    }
  }

  return nullptr;
}

void Decoder::Conform(const AudioRenderingParams &params, const QAtomicInt* cancelled)
{
  if (stream()->type() != Stream::kAudio) {
    // Nothing to be done
    return;
  }

  // Get indexed WAV file
  WaveInput input(GetIndexFilename());

  // FIXME: No handling if input failed to open/is corrupt
  if (input.open()) {
    // If the parameters are equal, nothing to be done
    // FIXME: Technically we only need to conform if the SAMPLE RATE is not equal. Format and channel layout conversion
    //        could be done on the fly so we could perhaps conform less often at some point.
    if (input.params() == params) {
      input.close();
      return;
    }

    // Otherwise, let's start converting the format
    QMutexLocker locker(stream()->index_process_lock());

    // Generate destination filename for this conversion to see if it exists
    QString conformed_fn = GetConformedFilename(params);

    if (QFileInfo::exists(conformed_fn)) {
      // We must have already conformed this format
      std::static_pointer_cast<AudioStream>(stream())->append_conformed_version(params);
      input.close();
      return;
    }

    // Set up resampler
    SwrContext* resampler = swr_alloc_set_opts(nullptr,
                                               static_cast<int64_t>(params.channel_layout()),
                                               FFmpegCommon::GetFFmpegSampleFormat(params.format()),
                                               params.sample_rate(),
                                               static_cast<int64_t>(input.params().channel_layout()),
                                               FFmpegCommon::GetFFmpegSampleFormat(input.params().format()),
                                               input.params().sample_rate(),
                                               0,
                                               nullptr);

    swr_init(resampler);

    WaveOutput conformed_output(conformed_fn, params);
    if (!conformed_output.open()) {
      qWarning() << "Failed to open conformed output:" << conformed_fn;
      input.close();
      return;
    }

    // Convert one second of audio at a time
    int input_buffer_sz = input.params().time_to_bytes(1);

    int read_count = 0;

    while (!input.at_end()) {
      if (cancelled && *cancelled) {
        break;
      }

      // Read up to one second of audio from WAV file
      QByteArray read_samples = input.read(input_buffer_sz);

      // Determine how many samples this is
      int in_sample_count = input.params().bytes_to_samples(read_samples.size());

      ConformInternal(resampler, &conformed_output, read_samples.data(), in_sample_count);

      read_count += read_samples.size();
      emit IndexProgress(qRound(100.0 * static_cast<double>(read_count) / static_cast<double>(input.data_length())));
    }

    // Flush resampler
    ConformInternal(resampler, &conformed_output, nullptr, 0);

    // Clean up
    swr_free(&resampler);
    conformed_output.close();
    input.close();

    // If we cancelled, the conform didn't finish so remove it
    if (cancelled && *cancelled) {
      QFile(conformed_fn).remove();
    } else {
      std::static_pointer_cast<AudioStream>(stream())->append_conformed_version(params);
    }
  } else {
    qWarning() << "Failed to conform file:" << stream()->footage()->filename();
  }
}

void Decoder::ConformInternal(SwrContext* resampler, WaveOutput* output, const char* in_data, int in_sample_count)
{
  // Determine how many samples the output will be
  int out_sample_count = swr_get_out_samples(resampler, in_sample_count);

  // Allocate array for the amount of samples we'll need
  QByteArray out_samples;
  out_samples.resize(output->params().samples_to_bytes(out_sample_count));

  char* out_data = out_samples.data();

  // Convert samples
  int convert_count = swr_convert(resampler,
                                  reinterpret_cast<uint8_t**>(&out_data),
                                  out_sample_count,
                                  reinterpret_cast<const uint8_t**>(&in_data),
                                  in_sample_count);

  if (convert_count != out_sample_count) {
    out_samples.resize(output->params().samples_to_bytes(convert_count));
  }

  output->write(out_samples);
}

QString Decoder::GetConformedFilename(const AudioRenderingParams &params)
{
  QString index_fn = GetIndexFilename();
  WaveInput input(GetIndexFilename());

  // FIXME: No handling if input failed to open/is corrupt
  if (input.open()) {
    // If the parameters are equal, nothing to be done
    AudioRenderingParams index_params = input.params();
    input.close();

    if (index_params == params) {
      // Source file matches perfectly, no conform required
      return index_fn;
    }
  }

  index_fn.append('.');
  index_fn.append(QString::number(params.sample_rate()));
  index_fn.append('.');
  index_fn.append(QString::number(params.format()));
  index_fn.append('.');
  index_fn.append(QString::number(params.channel_layout()));

  return index_fn;
}

void Decoder::ProxyVideo(const QAtomicInt *, int )
{
}

void Decoder::ProxyAudio(const QAtomicInt *)
{
}

bool Decoder::HasConformedVersion(const AudioRenderingParams &params)
{
  if (stream()->type() != Stream::kAudio) {
    return false;
  }

  AudioStreamPtr audio_stream = std::static_pointer_cast<AudioStream>(stream());

  if (audio_stream->has_conformed_version(params)) {
    return true;
  }

  // Get indexed WAV file
  WaveInput input(GetIndexFilename());

  bool index_already_matches = false;

  if (input.open()) {
    index_already_matches = (input.params() == params);

    input.close();
  }

  return index_already_matches;
}

void Decoder::SignalIndexProgress(const int64_t &ts)
{
  if (stream()->duration() != AV_NOPTS_VALUE && stream()->duration() != 0) {
    emit IndexProgress(qRound(100.0 * static_cast<double>(ts) / static_cast<double>(stream()->duration())));
  }
}

OLIVE_NAMESPACE_EXIT
