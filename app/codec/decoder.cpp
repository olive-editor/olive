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
#ifdef USE_OTIO
#include "task/project/loadotio/loadotio.h"
#endif
#include "task/taskmanager.h"
#include "project/project.h"

namespace olive {

QMutex Decoder::currently_conforming_mutex_;
QWaitCondition Decoder::currently_conforming_wait_cond_;
QVector<Decoder::CurrentlyConforming> Decoder::currently_conforming_;

const rational Decoder::kAnyTimecode = RATIONAL_MIN;

Decoder::Decoder()
{
}

bool Decoder::Open(const CodecStream &stream)
{
  QMutexLocker locker(&mutex_);

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

FramePtr Decoder::RetrieveVideo(const rational &timecode, const int &divider)
{
  QMutexLocker locker(&mutex_);

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

SampleBufferPtr Decoder::RetrieveAudio(const TimeRange &range, const AudioParams &params, const QString& cache_path, const QAtomicInt *cancelled)
{
  QMutexLocker locker(&mutex_);

  if (!stream_.IsValid()) {
    qCritical() << "Can't retrieve audio on a closed decoder";
    return nullptr;
  }

  if (!SupportsAudio()) {
    qCritical() << "Decoder doesn't support audio";
    return nullptr;
  }

  // Determine if we already have a conformed version
  QString conform_filename = GetConformedFilename(cache_path, params);
  CurrentlyConforming want_conform = {stream_, params};

  currently_conforming_mutex_.lock();

  // Wait for conform to complete
  while (currently_conforming_.contains(want_conform)) {
    currently_conforming_wait_cond_.wait(&currently_conforming_mutex_);
  }

  // See if we got the conform
  SampleBufferPtr buffer = RetrieveAudioFromConform(conform_filename, range);

  if (!buffer) {
    // We'll need to conform this ourselves
    currently_conforming_.append(want_conform);
    currently_conforming_mutex_.unlock();

    // We conform to a different filename until it's done to make it clear even across sessions
    // whether this conform is ready or not
    QString working_fn = conform_filename;
    working_fn.append(QStringLiteral(".working"));

    if (ConformAudioInternal(working_fn, params, cancelled)) {
      // Move file to standard conform name, making it clear this conform is ready for use
      QFile::remove(conform_filename);
      QFile::rename(working_fn, conform_filename);

      // Return audio as planned
      buffer = RetrieveAudioFromConform(conform_filename, range);
    } else {
      // Failed
      qCritical() << "Failed to conform audio";
    }

    currently_conforming_mutex_.lock();
    currently_conforming_.removeOne(want_conform);
    currently_conforming_wait_cond_.wakeAll();
  }

  currently_conforming_mutex_.unlock();

  return buffer;
}

void Decoder::Close()
{
  QMutexLocker locker(&mutex_);

  if (stream_.IsValid()) {
    CloseInternal();
    stream_.Reset();
  } else {
    qWarning() << "Tried to close a decoder that wasn't open";
  }
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

QString Decoder::GetConformedFilename(const QString& cache_path, const AudioParams &params)
{
  QString index_fn = QStringLiteral("%1.%2:%3").arg(FileFunctions::GetUniqueFileIdentifier(stream_.filename()),
                                                    QString::number(stream_.stream()));

  index_fn = QDir(cache_path).filePath(index_fn);

  index_fn.append('.');
  index_fn.append(QString::number(params.sample_rate()));
  index_fn.append('.');
  index_fn.append(QString::number(params.format()));
  index_fn.append('.');
  index_fn.append(QString::number(params.channel_layout()));

  return index_fn;
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

  QString original_basename = file_info.baseName();

  QString new_basename = original_basename.left(original_basename.size() - digit_count)
      .append(QStringLiteral("%1").arg(number, digit_count, 10, QChar('0')));

  return file_info.dir().filePath(file_info.fileName().replace(original_basename, new_basename));
}

int Decoder::GetImageSequenceDigitCount(const QString &filename)
{
  QString basename = QFileInfo(filename).baseName();

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

  QString original_basename = file_info.baseName();

  QString number_only = original_basename.mid(original_basename.size() - digit_count);

  return number_only.toLongLong();
}

FramePtr Decoder::RetrieveVideoInternal(const rational &timecode, const int &divider)
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

SampleBufferPtr Decoder::RetrieveAudioFromConform(const QString &conform_filename, const TimeRange& range)
{
  WaveInput input(conform_filename);

  if (input.open()) {
    const AudioParams& input_params = input.params();

    // Read bytes from wav
    QByteArray packed_data = input.read(input_params.time_to_bytes(range.in()),
                                        input_params.time_to_bytes(range.length()));
    input.close();

    // Create sample buffer
    SampleBufferPtr sample_buffer = SampleBuffer::CreateFromPackedData(input_params, packed_data);

    return sample_buffer;
  }

  return nullptr;
}

uint qHash(Decoder::CodecStream stream, uint seed)
{
  return qHash(stream.filename(), seed) ^ qHash(stream.stream(), seed);
}

}
