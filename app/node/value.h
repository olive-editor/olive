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

#ifndef NODEVALUE_H
#define NODEVALUE_H

#include <any>
#include <QMatrix4x4>

#include "render/texture.h"

namespace olive {

class Node;
class value_t;
class SampleJob;

using NodeValueArray = std::vector<value_t>;

class type_t
{
public:
  constexpr type_t() : type_(0) {}
  constexpr type_t(const char *x) : type_(insn_to_num(x)) {}

  static type_t fromString(const QStringView &s) { return s.toUtf8().constData(); }
  QString toString() const
  {
    const char *c = reinterpret_cast<const char*>(&type_);
    return QString::fromUtf8(c, strnlen(c, sizeof(type_)));
  }

  bool operator==(const type_t &t) const { return type_ == t.type_; }
  bool operator!=(const type_t &t) const { return !(*this == t); }
  bool operator<(const type_t &t) const { return type_ < t.type_; }
  bool operator<=(const type_t &t) const { return type_ <= t.type_; }
  bool operator>(const type_t &t) const { return type_ > t.type_; }
  bool operator>=(const type_t &t) const { return type_ >= t.type_; }

private:
  constexpr uint64_t insn_to_num(const char* x)
  {
    return *x ? *x + (insn_to_num(x+1) << 8) : 0;
  }

  uint64_t type_;

};

QDebug operator<<(QDebug dbg, const type_t &t);

constexpr type_t TYPE_NONE;
constexpr type_t TYPE_INTEGER = "int";
constexpr type_t TYPE_DOUBLE = "dbl";
constexpr type_t TYPE_RATIONAL = "rational";
constexpr type_t TYPE_STRING = "str";
constexpr type_t TYPE_TEXTURE = "tex";
constexpr type_t TYPE_SAMPLES = "smp";
constexpr type_t TYPE_BINARY = "bin";
constexpr type_t TYPE_ARRAY = "arr";
constexpr type_t TYPE_MATRIX = "mat";

class value_t
{
public:
  class component_t
  {
  public:
    component_t() = default;

    template <typename T>
    component_t(const T &t)
    {
      set(t);
    }

    // Bad initializers, must catch these at runtime
    component_t(const value_t &t) { abort(); }
    component_t(const QVariant &t) { abort(); }
    component_t(const std::vector<component_t> &t) { abort(); }

    template <typename T>
    T get() const
    {
      T t = T();

      if (data_.has_value()) {
        t = std::any_cast<T>(data_);
      }

      return t;
    }

    template <typename T>
    void set(const T &t)
    {
      data_ = t;
    }

    const std::any &data() const { return data_; }

    component_t converted(type_t from, type_t to, bool *ok = nullptr) const;

    static component_t fromSerializedString(type_t to, const QString &s);
    QString toSerializedString(type_t from) const;

  private:
    std::any data_;

  };

  value_t(const type_t &type, size_t channels = 1) :
    type_(type)
  {
    data_.resize(channels);
  }

  value_t(const type_t &type, const std::vector<value_t::component_t> &components) :
    type_(type)
  {
    data_ = components;
  }

  template <typename T>
  value_t(const type_t &type, const T &v) :
    value_t(type)
  {
    data_[0] = v;
  }

  value_t() :
    type_(TYPE_NONE)
  {}

  value_t(TexturePtr texture) :
    value_t(TYPE_TEXTURE, texture)
  {
  }

  value_t(const SampleJob &samples);

  value_t(const QVector2D &vec) :
    value_t(TYPE_DOUBLE, size_t(2))
  {
    data_[0] = double(vec.x());
    data_[1] = double(vec.y());
  }

  value_t(const QVector3D &vec) :
    value_t(TYPE_DOUBLE, size_t(3))
  {
    data_[0] = double(vec.x());
    data_[1] = double(vec.y());
    data_[2] = double(vec.z());
  }

  value_t(const QVector4D &vec) :
    value_t(TYPE_DOUBLE, size_t(4))
  {
    data_[0] = double(vec.x());
    data_[1] = double(vec.y());
    data_[2] = double(vec.z());
    data_[3] = double(vec.w());
  }

  value_t(const float &f) :
    value_t(TYPE_DOUBLE, double(f))
  {
  }

  value_t(const double &d) :
    value_t(TYPE_DOUBLE, d)
  {
  }

  value_t(const int64_t &i) :
    value_t(TYPE_INTEGER, i)
  {
  }

  value_t(const uint64_t &i) :
    value_t(TYPE_INTEGER, int64_t(i))
  {
  }

  value_t(const int &i) :
    value_t(TYPE_INTEGER, int64_t(i))
  {
  }

  value_t(const bool &i) :
    value_t(TYPE_INTEGER, int64_t(i))
  {
  }

  value_t(const Color &i) :
    value_t(TYPE_DOUBLE, size_t(4))
  {
    data_[0] = double(i.red());
    data_[1] = double(i.green());
    data_[2] = double(i.blue());
    data_[3] = double(i.alpha());
  }

  value_t(const QString &i) :
    value_t(TYPE_STRING, i)
  {
  }

  value_t(const char *i) :
    value_t(TYPE_STRING, QString::fromUtf8(i))
  {
  }

  value_t(const rational &i) :
    value_t(TYPE_RATIONAL, i)
  {
  }

  value_t(const QByteArray &i) :
    value_t(TYPE_BINARY, i)
  {
  }

  value_t(const NodeValueArray &i) :
    value_t(TYPE_ARRAY, i)
  {
  }

  value_t(const QMatrix4x4 &i) :
    value_t(TYPE_MATRIX, i)
  {
  }

  const type_t &type() const
  {
    return type_;
  }

  component_t at(size_t channel) const
  {
    if (channel < data_.size()) {
      return data_[channel];
    }
    return component_t();
  }

  component_t &operator[](size_t i) { return data_[i]; }
  const component_t &operator[](size_t i) const { return data_[i]; }

  template <typename T>
  T value(size_t channel = 0) const
  {
    return at(channel).get<T>();
  }

  void resize(size_t s) { data_.resize(s); }
  size_t size() const { return data_.size(); }

  std::vector<component_t> &data() { return data_; }
  const std::vector<component_t> &data() const { return data_; }

  bool isValid() const { return type_ != TYPE_NONE; }

  bool toBool() const { return toInt(); }
  double toDouble() const { return value<double>(); }
  int64_t toInt() const { return value<int64_t>(); }
  rational toRational() const { return value<olive::core::rational>(); }
  QString toString() const { return value<QString>(); }
  Color toColor() const { return Color(value<double>(0), value<double>(1), value<double>(2), value<double>(3)); }
  QVector2D toVec2() const { return QVector2D(value<double>(0), value<double>(1)); }
  QVector3D toVec3() const { return QVector3D(value<double>(0), value<double>(1), value<double>(2)); }
  QVector4D toVec4() const { return QVector4D(value<double>(0), value<double>(1), value<double>(2), value<double>(3)); }
  QMatrix4x4 toMatrix() const { return value<QMatrix4x4>(); }
  QByteArray toBinary() const { return value<QByteArray>(); }
  NodeValueArray toArray() const { return value<NodeValueArray>(); }
  TexturePtr toTexture() const { return value<TexturePtr>(); }
  SampleBuffer toSamples() const { return value<SampleBuffer>(); }

  static value_t fromSerializedString(type_t target_type, const QString &s);
  QString toSerializedString() const;

  value_t converted(type_t to, bool *ok = nullptr) const;

  typedef component_t(*Converter_t)(const component_t &v);
  static void registerConverter(const type_t &from, const type_t &to, Converter_t converter);
  static void registerDefaultConverters();

private:
  type_t type_;
  std::vector<component_t> data_;

  static std::map<type_t, std::map<type_t, Converter_t> > converters_;

};

}

Q_DECLARE_METATYPE(olive::value_t)

#endif // NODEVALUE_H
