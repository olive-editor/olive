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

#ifndef SEQUENCEPARAM_H
#define SEQUENCEPARAM_H

#include <QXmlStreamWriter>

#include "common/rational.h"
#include "common/xmlutils.h"
#include "dialog/sequence/presetmanager.h"
#include "render/videoparams.h"

namespace olive {

class SequencePreset : public Preset {
public:
  SequencePreset() = default;

  SequencePreset(const QString& name,
                 int width,
                 int height,
                 const rational& frame_rate,
                 const rational& pixel_aspect,
                 VideoParams::Interlacing interlacing,
                 int sample_rate,
                 uint64_t channel_layout,
                 int preview_divider,
                 VideoParams::Format preview_format) :
    width_(width),
    height_(height),
    frame_rate_(frame_rate),
    pixel_aspect_(pixel_aspect),
    interlacing_(interlacing),
    sample_rate_(sample_rate),
    channel_layout_(channel_layout),
    preview_divider_(preview_divider),
    preview_format_(preview_format)
  {
    SetName(name);
  }

  static PresetPtr Create(const QString& name,
                          int width,
                          int height,
                          const rational& frame_rate,
                          const rational& pixel_aspect,
                          VideoParams::Interlacing interlacing,
                          int sample_rate,
                          uint64_t channel_layout,
                          int preview_divider,
                          VideoParams::Format preview_format)
  {
    return std::make_shared<SequencePreset>(name, width, height, frame_rate, pixel_aspect,
                                            interlacing, sample_rate, channel_layout,
                                            preview_divider, preview_format);
  }

  virtual void Load(QXmlStreamReader* reader) override
  {
    while (XMLReadNextStartElement(reader)) {
      if (reader->name() == QStringLiteral("name")) {
        SetName(reader->readElementText());
      } else if (reader->name() == QStringLiteral("width")) {
        width_ = reader->readElementText().toInt();
      } else if (reader->name() == QStringLiteral("height")) {
        height_ = reader->readElementText().toInt();
      } else if (reader->name() == QStringLiteral("framerate")) {
        frame_rate_ = rational::fromString(reader->readElementText());
      } else if (reader->name() == QStringLiteral("pixelaspect")) {
        pixel_aspect_ = rational::fromString(reader->readElementText());
      } else if (reader->name() == QStringLiteral("interlacing")) {
        interlacing_ = static_cast<VideoParams::Interlacing>(reader->readElementText().toInt());
      } else if (reader->name() == QStringLiteral("samplerate")) {
        sample_rate_ = reader->readElementText().toInt();
      } else if (reader->name() == QStringLiteral("chlayout")) {
        channel_layout_ = reader->readElementText().toULongLong();
      } else if (reader->name() == QStringLiteral("divider")) {
        preview_divider_ = reader->readElementText().toInt();
      } else if (reader->name() == QStringLiteral("format")) {
        preview_format_ = static_cast<VideoParams::Format>(reader->readElementText().toInt());
      } else {
        reader->skipCurrentElement();
      }
    }
  }

  virtual void Save(QXmlStreamWriter* writer) const override
  {
    writer->writeTextElement(QStringLiteral("name"), GetName());
    writer->writeTextElement(QStringLiteral("width"), QString::number(width_));
    writer->writeTextElement(QStringLiteral("height"), QString::number(height_));
    writer->writeTextElement(QStringLiteral("framerate"), frame_rate_.toString());
    writer->writeTextElement(QStringLiteral("pixelaspect"), pixel_aspect_.toString());
    writer->writeTextElement(QStringLiteral("interlacing_"), QString::number(interlacing_));
    writer->writeTextElement(QStringLiteral("samplerate"), QString::number(sample_rate_));
    writer->writeTextElement(QStringLiteral("chlayout"), QString::number(channel_layout_));
    writer->writeTextElement(QStringLiteral("divider"), QString::number(preview_divider_));
    writer->writeTextElement(QStringLiteral("format"), QString::number(preview_format_));
  }

  int width() const
  {
    return width_;
  }

  int height() const
  {
    return height_;
  }

  const rational& frame_rate() const
  {
    return frame_rate_;
  }

  const rational& pixel_aspect() const
  {
    return pixel_aspect_;
  }

  VideoParams::Interlacing interlacing() const
  {
    return interlacing_;
  }

  int sample_rate() const
  {
    return sample_rate_;
  }

  uint64_t channel_layout() const
  {
    return channel_layout_;
  }

  int preview_divider() const
  {
    return preview_divider_;
  }

  VideoParams::Format preview_format() const
  {
    return preview_format_;
  }

private:
  int width_;
  int height_;
  rational frame_rate_;
  rational pixel_aspect_;
  VideoParams::Interlacing interlacing_;
  int sample_rate_;
  uint64_t channel_layout_;
  int preview_divider_;
  VideoParams::Format preview_format_;

};

}

#endif // SEQUENCEPARAM_H
