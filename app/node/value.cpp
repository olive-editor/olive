/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2023 Olive Studios LLC

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

#include "value.h"

#include <QCoreApplication>
#include <QMatrix4x4>
#include <QVector2D>
#include <QVector3D>
#include <QVector4D>

#include "common/tohex.h"
#include "render/job/samplejob.h"
#include "render/job/shaderjob.h"
#include "render/subtitleparams.h"
#include "render/videoparams.h"

namespace olive {

std::map<type_t, std::map<type_t, value_t::Converter_t> > value_t::converters_;

QChar CHANNEL_SPLITTER = ':';

struct TextureChannel
{
  TexturePtr texture;
  size_t channel;
};

struct SampleJobChannel
{
  AudioJobPtr job;
  size_t channel;
};

value_t::value_t(TexturePtr texture) :
  value_t(TYPE_TEXTURE)
{
  if (texture) {
    size_t sz = texture->channel_count();
    data_.resize(sz);
    for (size_t i = 0; i < sz; i++) {
      data_[i] = TextureChannel({texture, i});
    }
  }
}

value_t::value_t(AudioJobPtr job)
{
  if (job) {
    size_t sz = job->params().channel_count();
    data_.resize(sz);
    for (size_t i = 0; i < sz; i++) {
      data_[i] = SampleJobChannel({job, i});
    }
    type_ = TYPE_SAMPLES;
  }
}

value_t::value_t(const SampleJob &job) :
  value_t(AudioJob::Create(job.audio_params(), job))
{
}

ShaderCode GetSwizzleShaderCode(const QString &id)
{
  return ShaderCode(FileFunctions::ReadFileAsString(QStringLiteral(":/shaders/swizzle.frag")));
}

void SwizzleAudio(const void *context, const SampleJob &job, SampleBuffer &output)
{
  for (int i = 0; i < output.channel_count(); i++) {
    SampleBuffer b = job.Get(QStringLiteral("b.%1").arg(i)).toSamples();
    if (b.is_allocated()) {
      size_t c = job.Get(QStringLiteral("c.%1").arg(i)).toInt();
      output.fast_set(b, i, c);
    }
  }
}

TexturePtr value_t::toTexture() const
{
  if (type_ != TYPE_TEXTURE || data_.empty()) {
    return TexturePtr();
  }

  bool swizzled = false;

  TextureChannel last;
  VideoParams vp;

  for (auto it = data_.cbegin(); it != data_.cend(); it++) {
    const TextureChannel &c = it->value<TextureChannel>();

    if (it != data_.cbegin() && !swizzled) {
      if (c.texture != last.texture || c.channel != last.channel + 1) {
        swizzled = true;
      }
    }

    if (!vp.is_valid() && c.texture && c.texture->params().is_valid()) {
      vp = c.texture->params();
    }

    last = c;
  }

  if (swizzled) {
    // Return texture(s) wrapped in a swizzle shader
    ShaderJob job;
    job.set_function(GetSwizzleShaderCode);
    job.Insert(QStringLiteral("red_texture"), this->value<TextureChannel>(0).texture);
    job.Insert(QStringLiteral("green_texture"), this->value<TextureChannel>(1).texture);
    job.Insert(QStringLiteral("blue_texture"), this->value<TextureChannel>(2).texture);
    job.Insert(QStringLiteral("alpha_texture"), this->value<TextureChannel>(3).texture);
    job.Insert(QStringLiteral("red_channel"), this->value<TextureChannel>(0).channel);
    job.Insert(QStringLiteral("green_channel"), this->value<TextureChannel>(1).channel);
    job.Insert(QStringLiteral("blue_channel"), this->value<TextureChannel>(2).channel);
    job.Insert(QStringLiteral("alpha_channel"), this->value<TextureChannel>(3).channel);
    return Texture::Job(vp, job);
  } else {
    // No swizzling has occurred
    return last.texture;
  }
}

AudioJobPtr value_t::toAudioJob() const
{
  if (type_ != TYPE_SAMPLES || data_.empty()) {
    return AudioJobPtr();
  }

  bool swizzled = false;

  SampleJobChannel last;
  AudioParams ap;
  TimeRange time;

  for (auto it = data_.cbegin(); it != data_.cend(); it++) {
    const SampleJobChannel &c = it->value<SampleJobChannel>();

    if (it != data_.cbegin() && !swizzled) {
      if (c.job != last.job || c.channel != last.channel + 1) {
        swizzled = true;
      }
    }

    if (c.job) {
      if (!ap.is_valid() && c.job->params().is_valid()) {
        ap = c.job->params();
      }

      if (time.length().isNull()) {
        time = c.job->time();
      }
    }

    last = c;
  }

  if (swizzled) {
    if (ap.is_valid()) {
      // Return texture(s) wrapped in a swizzle shader
      ap.set_channel_layout(av_get_default_channel_layout(data_.size()));

      SampleJob swizzle(ValueParams(VideoParams(), ap, time, QString(), LoopMode(), nullptr, nullptr));

      for (size_t i = 0; i < data_.size(); i++) {
        const SampleJobChannel &c = data_.at(i).value<SampleJobChannel>();
        swizzle.Insert(QStringLiteral("b.%1").arg(i), c.job);
        swizzle.Insert(QStringLiteral("c.%1").arg(i), c.channel);
      }

      swizzle.set_function(SwizzleAudio, nullptr);

      return AudioJob::Create(ap, swizzle);
    } else {
      return nullptr;
    }
  } else {
    // No swizzling has occurred
    return last.job;
  }
}

value_t value_t::fromSerializedString(type_t target_type, const QString &str)
{
  QStringList l = str.split(CHANNEL_SPLITTER);
  value_t v(TYPE_STRING, l.size());

  for (size_t i = 0; i < v.size(); i++) {
    v[i] = l[i];
  }

  if (target_type != TYPE_STRING) {
    v = v.converted(target_type);
  }

  return v;
}

QString value_t::toSerializedString() const
{
  if (this->type() == TYPE_STRING) {
    QStringList l;
    for (auto it = data_.cbegin(); it != data_.cend(); it++) {
      l.append(it->value<QString>());
    }
    return l.join(CHANNEL_SPLITTER);
  } else {
    bool ok;
    value_t stringified = this->converted(TYPE_STRING, &ok);
    if (ok) {
      return stringified.toSerializedString();
    } else {
      return QString();
    }
  }
}

value_t value_t::converted(type_t to, bool *ok) const
{
  // No-op if type is already requested
  if (this->type() == to) {
    if (ok) *ok = true;
    return *this;
  }

  // Find converter, no-op if none found
  Converter_t c = converters_[this->type()][to];
  if (!c) {
    if (ok) *ok = false;
    return *this;
  }

  // Create new value with new type and same amount of channels
  value_t v(to, data_.size());

  // Run each channel through converter
  for (size_t i = 0; i < data_.size(); i++) {
    v.data_[0] = c(data_[0]);
  }

  if (ok) *ok = true;
  return v;
}

value_t::component_t converter_IntegerToString(const value_t::component_t &v)
{
  return QString::number(v.value<int64_t>());
}

value_t::component_t converter_StringToInteger(const value_t::component_t &v)
{
  return int64_t(v.value<QString>().toLongLong());
}

value_t::component_t converter_DoubleToString(const value_t::component_t &v)
{
  return QString::number(v.value<double>());
}

value_t::component_t converter_StringToDouble(const value_t::component_t &v)
{
  return v.value<QString>().toDouble();
}

value_t::component_t converter_RationalToString(const value_t::component_t &v)
{
  return QString::fromStdString(v.value<rational>().toString());
}

value_t::component_t converter_StringToRational(const value_t::component_t &v)
{
  return rational::fromString(v.value<QString>().toStdString());
}

value_t::component_t converter_BinaryToString(const value_t::component_t &v)
{
  return QString::fromLatin1(v.value<QByteArray>().toBase64());
}

value_t::component_t converter_StringToBinary(const value_t::component_t &v)
{
  return QByteArray::fromBase64(v.value<QString>().toLatin1());
}

value_t::component_t converter_IntegerToDouble(const value_t::component_t &v)
{
  return double(v.value<int64_t>());
}

value_t::component_t converter_IntegerToRational(const value_t::component_t &v)
{
  return rational(v.value<int64_t>());
}

value_t::component_t converter_DoubleToInteger(const value_t::component_t &v)
{
  return int64_t(v.value<double>());
}

value_t::component_t converter_DoubleToRational(const value_t::component_t &v)
{
  return rational::fromDouble(v.value<double>());
}

value_t::component_t converter_RationalToInteger(const value_t::component_t &v)
{
  return int64_t(v.value<rational>().toDouble());
}

value_t::component_t converter_RationalToDouble(const value_t::component_t &v)
{
  return v.value<rational>().toDouble();
}

void value_t::registerConverter(const type_t &from, const type_t &to, Converter_t converter)
{
  if (converters_[from][to]) {
    qWarning() << "Failed to register converter from" << from.toString() << "to" << to.toString() << "- converter already exists";
    return;
  }

  converters_[from][to] = converter;
}

void value_t::registerDefaultConverters()
{
  registerConverter(TYPE_INTEGER, TYPE_STRING, converter_IntegerToString);
  registerConverter(TYPE_DOUBLE, TYPE_STRING, converter_DoubleToString);
  registerConverter(TYPE_RATIONAL, TYPE_STRING, converter_RationalToString);
  registerConverter(TYPE_BINARY, TYPE_STRING, converter_BinaryToString);

  registerConverter(TYPE_STRING, TYPE_INTEGER, converter_StringToInteger);
  registerConverter(TYPE_STRING, TYPE_DOUBLE, converter_StringToDouble);
  registerConverter(TYPE_STRING, TYPE_RATIONAL, converter_StringToRational);
  registerConverter(TYPE_STRING, TYPE_BINARY, converter_StringToBinary);

  registerConverter(TYPE_INTEGER, TYPE_DOUBLE, converter_IntegerToDouble);
  registerConverter(TYPE_INTEGER, TYPE_RATIONAL, converter_IntegerToRational);

  registerConverter(TYPE_DOUBLE, TYPE_INTEGER, converter_DoubleToInteger);
  registerConverter(TYPE_DOUBLE, TYPE_RATIONAL, converter_DoubleToRational);

  registerConverter(TYPE_RATIONAL, TYPE_INTEGER, converter_RationalToInteger);
  registerConverter(TYPE_RATIONAL, TYPE_DOUBLE, converter_RationalToDouble);
}

value_t::component_t value_t::component_t::converted(type_t from, type_t to, bool *ok) const
{
  if (Converter_t c = converters_[from][to]) {
    if (ok) *ok = true;
    return c(*this);
  }

  if (ok) *ok = false;
  return *this;
}

value_t::component_t value_t::component_t::fromSerializedString(type_t to, const QString &str)
{
  component_t c = str;
  if (to != TYPE_STRING) {
    c = c.converted(TYPE_STRING, to);
  }
  return c;
}

QString value_t::component_t::toSerializedString(type_t from) const
{
  if (from == TYPE_STRING) {
    return value<QString>();
  } else {
    component_t c = this->converted(from, TYPE_STRING);
    return c.value<QString>();
  }
}

QDebug operator<<(QDebug dbg, const type_t &t)
{
  return dbg << t.toString();
}

}
