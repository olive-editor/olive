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
#include <QFileInfo>

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

Decoder::Decoder() :
  stream_(nullptr)
{
}

bool Decoder::Open(StreamPtr fs)
{
  QMutexLocker locker(&mutex_);

  if (stream_) {
    // Decoder is already open. Return TRUE if the stream is the stream we have, or FALSE if not.
    if (stream_ == fs) {
      return true;
    } else {
      qWarning() << "Tried to open a decoder that was already open with another stream";
      return false;
    }
  } else {
    // Stream was not open, try opening it now
    if (fs == nullptr) {
      // Cannot open null stream
      qCritical() << "Decoder attempted to open null stream";
      return false;
    }

    if (fs->footage()->decoder() != id()) {
      qCritical() << "Tried to open footage in incorrect decoder";
      return false;
    }

    // Set stream
    stream_ = fs;

    // Try open internal
    if (OpenInternal()) {
      return true;
    } else {
      // Unset stream
      CloseInternal();
      stream_ = nullptr;
      return false;
    }
  }
}

FramePtr Decoder::RetrieveVideo(const rational &timecode, const int &divider)
{
  QMutexLocker locker(&mutex_);

  if (!stream_) {
    qCritical() << "Can't retrieve video on a closed decoder";
    return nullptr;
  }

  if (!SupportsVideo()) {
    qCritical() << "Decoder doesn't support video";
    return nullptr;
  }

  if (stream_->type() != Stream::kVideo) {
    qCritical() << "Tried to retrieve video from a non-video stream";
    return nullptr;
  }

  return RetrieveVideoInternal(timecode, divider);
}

SampleBufferPtr Decoder::RetrieveAudio(const TimeRange &range, const AudioParams &params, const QAtomicInt *cancelled)
{
  QMutexLocker locker(&mutex_);

  if (!stream_) {
    qCritical() << "Can't retrieve audio on a closed decoder";
    return nullptr;
  }

  if (!SupportsAudio()) {
    qCritical() << "Decoder doesn't support audio";
    return nullptr;
  }

  if (stream_->type() != Stream::kAudio) {
    qCritical() << "Tried to retrieve audio from a non-audio stream";
    return nullptr;
  }

  // Determine if we already have a conformed version
  QString conform_filename = GetConformedFilename(params);
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

  if (stream_) {
    CloseInternal();
    stream_ = nullptr;
  } else {
    qWarning() << "Tried to close a decoder that wasn't open";
  }
}

/*
 * DECODER STATIC PUBLIC MEMBERS
 */

QVector<DecoderPtr> ReceiveListOfAllDecoders()
{
  QVector<DecoderPtr> decoders;

  // The order in which these decoders are added is their priority when probing. Hence FFmpeg should usually be last,
  // since it supports so many formats and we presumably want to override those formats with a more specific decoder.
  decoders.append(std::make_shared<OIIODecoder>());
  decoders.append(std::make_shared<FFmpegDecoder>());

  return decoders;
}

FootagePtr Decoder::Probe(Project* project, const QString &filename, const QAtomicInt* cancelled)
{
  // Check for a valid filename
  if (filename.isEmpty()) {
    qWarning() << "Tried to probe media with an empty filename";
    return nullptr;
  }

  // Check file exists
  if (!QFileInfo::exists(filename)) {
    qWarning() << "Tried to probe file that doesn't exist:" << filename;
    return nullptr;
  }

  // Create list to iterate through
  QVector<DecoderPtr> decoder_list = ReceiveListOfAllDecoders();

  // Pass Footage through each Decoder's probe function
  for (int i=0;i<decoder_list.size();i++) {

    if (cancelled && *cancelled) {
      return nullptr;
    }

    DecoderPtr decoder = decoder_list.at(i);

    FootagePtr footage = decoder->Probe(filename, cancelled);

    if (footage) {
      QFileInfo file_info(filename);
      footage->set_name(file_info.fileName());
      footage->set_filename(filename);

      footage->set_decoder(decoder->id());
      footage->set_project(project);
      footage->set_timestamp(file_info.lastModified().toMSecsSinceEpoch());

      footage->SetValid();

      // FIXME: Cache the results so we don't have to probe if this media is added a second time

      return footage;
    }
  }

  // We aren't able to use this Footage
  return nullptr;
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

QString Decoder::GetConformedFilename(const AudioParams &params)
{
  QString index_fn = GetIndexFilename();

  index_fn.append('.');
  index_fn.append(QString::number(params.sample_rate()));
  index_fn.append('.');
  index_fn.append(QString::number(params.format()));
  index_fn.append('.');
  index_fn.append(QString::number(params.channel_layout()));

  return index_fn;
}

QString Decoder::GetIndexFilename()
{
  return QDir(stream_->footage()->project()->cache_path()).filePath(FileFunctions::GetUniqueFileIdentifier(stream()->footage()->filename()).append(QString::number(stream()->index())));
}

void Decoder::SignalProcessingProgress(const int64_t &ts)
{
  if (stream()->duration() != AV_NOPTS_VALUE && stream()->duration() != 0) {
    emit IndexProgress(static_cast<double>(ts) / static_cast<double>(stream()->duration()));
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

}
