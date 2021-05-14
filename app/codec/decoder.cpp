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

#include "decoder.h"

#include <QCoreApplication>
#include <QDebug>

#include "codec/ffmpeg/ffmpegdecoder.h"
#include "codec/oiio/oiiodecoder.h"
#include "codec/waveinput.h"
#include "codec/waveoutput.h"
#include "common/ffmpegutils.h"
#include "common/filefunctions.h"
#include "common/timecodefunctions.h"
#include "conformmanager.h"
#include "node/project/project.h"
#include "task/taskmanager.h"

namespace olive {

const rational Decoder::kAnyTimecode = RATIONAL_MIN;

Decoder::Decoder()
{
  UpdateLastAccessed();
}

bool Decoder::Open(const CodecStream &stream)
{
  QMutexLocker locker(&mutex_);

  UpdateLastAccessed();

  if (stream_.IsValid()) {
    // Decoder is already open. Return TRUE if the stream is the stream we have, or FALSE if not.
    if (stream_ == stream) {
      return true;
    } else {
      qWarning() << "Tried to open a decoder that was already open with another stream";
      return false;
    }
  } else {
    // Stream was not open, try opening it now
    if (!stream.IsValid()) {
      // Cannot open null stream
      qCritical() << "Decoder attempted to open null stream";
      return false;
    }

    if (!stream.Exists()) {
      // Cannot open file that doesn't exist
      qCritical() << "Decoder attempted to open file that doesn't exist";
      return false;
    }

    // Set stream
    stream_ = stream;

    // Try open internal
    if (OpenInternal()) {
      return true;
    } else {
      // Unset stream
      qCritical() << "Failed to open" << stream_.filename() << "stream" << stream_.stream();
      CloseInternal();
      stream_.Reset();
      return false;
    }
  }
}

FramePtr Decoder::RetrieveVideo(const rational &timecode, const RetrieveVideoParams &divider)
{
  QMutexLocker locker(&mutex_);

  UpdateLastAccessed();

  if (!stream_.IsValid()) {
    qCritical() << "Can't retrieve video on a closed decoder";
    return nullptr;
  }

  if (!SupportsVideo()) {
    qCritical() << "Decoder doesn't support video";
    return nullptr;
  }

  return RetrieveVideoInternal(timecode, divider);
}

Decoder::RetrieveAudioData Decoder::RetrieveAudio(const TimeRange &range, const AudioParams &params, const QString& cache_path, Footage::LoopMode loop_mode, RenderMode::Mode mode)
{
  QMutexLocker locker(&mutex_);

  UpdateLastAccessed();

  if (!stream_.IsValid()) {
    qCritical() << "Can't retrieve audio on a closed decoder";
    return {kInvalid, nullptr, nullptr};
  }

  if (!SupportsAudio()) {
    qCritical() << "Decoder doesn't support audio";
    return {kInvalid, nullptr, nullptr};
  }

  // Get conform state from ConformManager
  ConformManager::Conform conform = ConformManager::instance()->GetConformState(id(), cache_path, stream_, params, (mode == RenderMode::kOnline));
  if (conform.state == ConformManager::kConformGenerating) {
    return {kWaitingForConform, nullptr, conform.task};
  }

  // See if we got the conform
  SampleBufferPtr out_buffer = RetrieveAudioFromConform(conform.filename, range, loop_mode);

  return {kOK, out_buffer, nullptr};
}

qint64 Decoder::GetLastAccessedTime()
{
  QMutexLocker locker(&mutex_);
  return last_accessed_;
}

void Decoder::Close()
{
  QMutexLocker locker(&mutex_);

  UpdateLastAccessed();

  if (stream_.IsValid()) {
    CloseInternal();
    stream_.Reset();
  } else {
    qWarning() << "Tried to close a decoder that wasn't open";
  }
}

bool Decoder::ConformAudio(const QString &output_filename, const AudioParams &params, const QAtomicInt *cancelled)
{
  return ConformAudioInternal(output_filename, params, cancelled);
}

/*
 * DECODER STATIC PUBLIC MEMBERS
 */

QVector<DecoderPtr> Decoder::ReceiveListOfAllDecoders()
{
  QVector<DecoderPtr> decoders;

  // The order in which these decoders are added is their priority when probing. Hence FFmpeg should usually be last,
  // since it supports so many formats and we presumably want to override those formats with a more specific decoder.
  decoders.append(std::make_shared<OIIODecoder>());
  decoders.append(std::make_shared<FFmpegDecoder>());

  return decoders;
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

int64_t Decoder::GetTimeInTimebaseUnits(const rational &time, const rational &timebase, int64_t start_time)
{
  return Timecode::time_to_timestamp(time, timebase) + start_time;
}

void Decoder::SignalProcessingProgress(int64_t ts, int64_t duration)
{
  if (duration != AV_NOPTS_VALUE && duration != 0) {
    emit IndexProgress(static_cast<double>(ts) / static_cast<double>(duration));
  }
}

QString Decoder::TransformImageSequenceFileName(const QString &filename, const int64_t& number)
{
  int digit_count = GetImageSequenceDigitCount(filename);

  QFileInfo file_info(filename);

  QString original_basename = file_info.completeBaseName();

  QString new_basename = original_basename.left(original_basename.size() - digit_count)
      .append(QStringLiteral("%1").arg(number, digit_count, 10, QChar('0')));

  return file_info.dir().filePath(file_info.fileName().replace(original_basename, new_basename));
}

int Decoder::GetImageSequenceDigitCount(const QString &filename)
{
  QString basename = QFileInfo(filename).completeBaseName();

  // See if basename contains a number at the end
  int digit_count = 0;

  for (int i=basename.size()-1;i>=0;i--) {
    if (basename.at(i).isDigit()) {
      digit_count++;
    } else {
      break;
    }
  }

  return digit_count;
}

int64_t Decoder::GetImageSequenceIndex(const QString &filename)
{
  int digit_count = GetImageSequenceDigitCount(filename);

  QFileInfo file_info(filename);

  QString original_basename = file_info.completeBaseName();

  QString number_only = original_basename.mid(original_basename.size() - digit_count);

  return number_only.toLongLong();
}

FramePtr Decoder::RetrieveVideoInternal(const rational &timecode, const RetrieveVideoParams &divider)
{
  Q_UNUSED(timecode)
  Q_UNUSED(divider)
  return nullptr;
}

bool Decoder::ConformAudioInternal(const QString& filename, const AudioParams &params, const QAtomicInt* cancelled)
{
  Q_UNUSED(filename)
  Q_UNUSED(cancelled)
  Q_UNUSED(params)
  return false;
}

SampleBufferPtr Decoder::RetrieveAudioFromConform(const QString &conform_filename, const TimeRange& range, Footage::LoopMode loop_mode)
{
  WaveInput input(conform_filename);

  if (input.open()) {
    const AudioParams& input_params = input.params();

    QByteArray packed_data(input_params.time_to_bytes(range.length()), Qt::Uninitialized);

    qint64 read_index = input_params.time_to_bytes(range.in());
    qint64 write_index = 0;

    while (write_index < packed_data.size()) {
      if (loop_mode == Footage::kLoopModeLoop) {
        while (read_index >= input.data_length()) {
          read_index -= input.data_length();
        }

        while (read_index < 0) {
          read_index += input.data_length();
        }
      }

      qint64 write_count = 0;

      if (read_index < 0) {
        // Reading before 0, write silence here until audio data would actually start
        write_count = qMin(-read_index, qint64(packed_data.size()));
        memset(packed_data.data() + write_index, 0, write_count);
      } else if (read_index >= input.data_length()) {
        // Reading after data length, write silence until the end of the buffer
        write_count = packed_data.size() - write_index;
        memset(packed_data.data() + write_index, 0, write_count);
      } else {
        write_count = qMin(input.data_length() - read_index, packed_data.size() - write_index);
        input.read(read_index, packed_data.data() + write_index, write_count);
      }

      read_index += write_count;
      write_index += write_count;
    }

    input.close();

    // Create sample buffer
    SampleBufferPtr sample_buffer = SampleBuffer::CreateFromPackedData(input_params, packed_data);

    return sample_buffer;
  }

  return nullptr;
}

void Decoder::UpdateLastAccessed()
{
  last_accessed_ = QDateTime::currentMSecsSinceEpoch();
}

uint qHash(Decoder::CodecStream stream, uint seed)
{
  return qHash(stream.filename(), seed) ^ qHash(stream.stream(), seed);
}

}
