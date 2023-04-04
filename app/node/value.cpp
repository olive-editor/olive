/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2022 Olive Team

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
#include "render/subtitleparams.h"
#include "render/videoparams.h"

namespace olive {

std::map<type_t, std::map<type_t, value_t::Converter_t> > value_t::converters_;

QChar CHANNEL_SPLITTER = ':';

value_t::value_t(const SampleJob &samples) :
  value_t(TYPE_SAMPLES, samples)
{
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
      l.append(it->get<QString>());
    }
    return l.join(CHANNEL_SPLITTER);
  } else {
    value_t stringified = this->converted(TYPE_STRING);
    return stringified.toSerializedString();
  }
}

value_t value_t::converted(type_t to) const
{
  // No-op if type is already requested
  if (this->type() == to) {
    return *this;
  }

  // Find converter, no-op if none found
  Converter_t c = converters_[this->type()][to];
  if (!c) {
    return *this;
  }

  // Create new value with new type and same amount of channels
  value_t v(to, data_.size());

  // Run each channel through converter
  for (size_t i = 0; i < data_.size(); i++) {
    v.data_[0] = c(data_[0]);
  }

  return v;
}

value_t::component_t converter_IntegerToString(const value_t::component_t &v)
{
  return QString::number(v.get<int64_t>());
}

value_t::component_t converter_StringToInteger(const value_t::component_t &v)
{
  return int64_t(v.get<QString>().toLongLong());
}

value_t::component_t converter_DoubleToString(const value_t::component_t &v)
{
  return QString::number(v.get<double>());
}

value_t::component_t converter_StringToDouble(const value_t::component_t &v)
{
  return v.get<QString>().toDouble();
}

value_t::component_t converter_RationalToString(const value_t::component_t &v)
{
  return QString::fromStdString(v.get<rational>().toString());
}

value_t::component_t converter_StringToRational(const value_t::component_t &v)
{
  return rational::fromString(v.get<QString>().toStdString());
}

value_t::component_t converter_BinaryToString(const value_t::component_t &v)
{
  return QString::fromLatin1(v.get<QByteArray>().toBase64());
}

value_t::component_t converter_StringToBinary(const value_t::component_t &v)
{
  return QByteArray::fromBase64(v.get<QString>().toLatin1());
}

value_t::component_t converter_IntegerToDouble(const value_t::component_t &v)
{
  return double(v.get<int64_t>());
}

value_t::component_t converter_IntegerToRational(const value_t::component_t &v)
{
  return rational(v.get<int64_t>());
}

value_t::component_t converter_DoubleToInteger(const value_t::component_t &v)
{
  return int64_t(v.get<double>());
}

value_t::component_t converter_DoubleToRational(const value_t::component_t &v)
{
  return rational::fromDouble(v.get<double>());
}

value_t::component_t converter_RationalToInteger(const value_t::component_t &v)
{
  return int64_t(v.get<rational>().toDouble());
}

value_t::component_t converter_RationalToDouble(const value_t::component_t &v)
{
  return v.get<rational>().toDouble();
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

value_t::component_t value_t::component_t::converted(type_t from, type_t to) const
{
  if (Converter_t c = converters_[from][to]) {
    return c(*this);
  }

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
    return get<QString>();
  } else {
    component_t c = this->converted(from, TYPE_STRING);
    return c.get<QString>();
  }
}

QDebug operator<<(QDebug dbg, const type_t &t)
{
  return dbg << t.toString();
}

}
