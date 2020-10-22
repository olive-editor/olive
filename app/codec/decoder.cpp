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
#ifdef USE_OTIO
#include "codec/otio/otiodecoder.h"
#endif
#include "codec/waveinput.h"
#include "codec/waveoutput.h"
#include "common/filefunctions.h"
#include "common/timecodefunctions.h"
#include "task/taskmanager.h"
#include "project/project.h"

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

StreamPtr Decoder::stream() const
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

SampleBufferPtr Decoder::RetrieveAudio(const rational &/*timecode*/, const rational &/*length*/, const AudioParams &/*params*/)
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
#ifdef USE_OTIO
  decoders.append(std::make_shared<OTIODecoder>());
#endif
  decoders.append(std::make_shared<OIIODecoder>());
  decoders.append(std::make_shared<FFmpegDecoder>());

  return decoders;
}

ItemPtr Decoder::ProbeMedia(const QString &filename, const QAtomicInt* cancelled)
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

    ItemPtr item = decoder->Probe(filename, cancelled);

    if (item) {

      if (item->type() == Item::kFootage) {
        // Attach the successful Decoder to this Footage object
        FootagePtr footage = std::static_pointer_cast<Footage>(item);
        footage->set_decoder(decoder->id());
        footage->SetValid();
      }


      // FIXME: Cache the results so we don't have to probe if this media is added a second time

      return item;
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

bool Decoder::ConformAudio(const QAtomicInt *, const AudioParams& )
{
  return false;
}

bool Decoder::HasConformedVersion(const AudioParams &params)
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

OLIVE_NAMESPACE_EXIT
