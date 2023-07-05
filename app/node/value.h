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

#ifndef NODEVALUE_H
#define NODEVALUE_H

#include <any>
#include <QMatrix4x4>

#include "render/job/audiojob.h"
#include "render/samplebuffer.h"
#include "render/texture.h"
#include "type.h"
#include "util/color.h"

namespace olive {

class Node;
class value_t;
class SampleJob;

using NodeValueArray = std::vector<value_t>;

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
    component_t(const T &t, const type_t &id = type_t())
    {
      set(t);
      id_ = id;
    }

    // Bad initializers, must catch these at runtime
    component_t(const value_t &t) { abort(); }
    component_t(const QVariant &t) { abort(); }
    component_t(const std::vector<component_t> &t) { abort(); }

    template <typename T>
    bool get(T *out) const
    {
      if (data_.has_value()) {
        try {
          *out = std::any_cast<T>(data_);
          return true;
        } catch (std::bad_any_cast &) {}
      }

      return false;
    }

    template <typename T>
    T value() const
    {
      T t = T();

      if (data_.has_value()) {
        try {
          t = std::any_cast<T>(data_);
        } catch (std::bad_any_cast &e) {
          qCritical() << "Failed to cast" << data_.type().name() << "to" << typeid(T).name() << e.what();
        }
      }

      return t;
    }

    template <typename T>
    void set(const T &t)
    {
      data_ = t;
    }

    const std::any &data() const { return data_; }
    const type_t &id() const { return id_; }
    void set_id(const type_t &id) { id_ = id; }

    component_t converted(type_t from, type_t to, bool *ok = nullptr) const;

    static component_t fromSerializedString(type_t to, const QString &s);
    QString toSerializedString(type_t from) const;

  private:
    std::any data_;
    type_t id_;

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

  value_t(TexturePtr texture);

  value_t(AudioJobPtr job);

  value_t(const SampleJob &job);

  value_t(const SampleBuffer &samples) :
    value_t(TYPE_SAMPLES, samples)
  {
  }

  value_t(const QVector2D &vec) :
    value_t(TYPE_DOUBLE, size_t(2))
  {
    data_[0] = component_t(double(vec.x()), XYZW_IDS.at(0));
    data_[1] = component_t(double(vec.y()), XYZW_IDS.at(1));
  }

  value_t(const QVector3D &vec) :
    value_t(TYPE_DOUBLE, size_t(3))
  {
    data_[0] = component_t(double(vec.x()), XYZW_IDS.at(0));
    data_[1] = component_t(double(vec.y()), XYZW_IDS.at(1));
    data_[2] = component_t(double(vec.z()), XYZW_IDS.at(2));
  }

  value_t(const QVector4D &vec) :
    value_t(TYPE_DOUBLE, size_t(4))
  {
    data_[0] = component_t(double(vec.x()), XYZW_IDS.at(0));
    data_[1] = component_t(double(vec.y()), XYZW_IDS.at(1));
    data_[2] = component_t(double(vec.z()), XYZW_IDS.at(2));
    data_[3] = component_t(double(vec.w()), XYZW_IDS.at(3));
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
    data_[0] = component_t(double(i.red()), RGBA_IDS.at(0));
    data_[1] = component_t(double(i.green()), RGBA_IDS.at(1));
    data_[2] = component_t(double(i.blue()), RGBA_IDS.at(2));
    data_[3] = component_t(double(i.alpha()), RGBA_IDS.at(3));
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
  bool get(T *out, size_t channel = 0) const
  {
    return at(channel).get<T>(out);
  }

  template <typename T>
  T value(size_t channel = 0) const
  {
    return at(channel).value<T>();
  }

  void resize(size_t s) { data_.resize(s); }
  size_t size() const { return data_.size(); }

  std::vector<component_t> &data() { return data_; }
  const std::vector<component_t> &data() const { return data_; }

  bool isValid() const { return type_ != TYPE_NONE && !data_.empty(); }

  bool toBool() const { return toInt(); }
  double toDouble() const { return value<double>(); }
  int64_t toInt() const { return value<int64_t>(); }
  rational toRational() const { return value<olive::rational>(); }
  QString toString() const;
  Color toColor() const { return Color(value<double>(0), value<double>(1), value<double>(2), value<double>(3)); }
  QVector2D toVec2() const { return QVector2D(value<double>(0), value<double>(1)); }
  QVector3D toVec3() const { return QVector3D(value<double>(0), value<double>(1), value<double>(2)); }
  QVector4D toVec4() const { return QVector4D(value<double>(0), value<double>(1), value<double>(2), value<double>(3)); }
  QMatrix4x4 toMatrix() const { return value<QMatrix4x4>(); }
  QByteArray toBinary() const { return value<QByteArray>(); }
  NodeValueArray toArray() const { return value<NodeValueArray>(); }
  TexturePtr toTexture() const;
  SampleBuffer toSamples() const { return value<SampleBuffer>(); }
  AudioJobPtr toAudioJob() const;

  static value_t fromSerializedString(type_t target_type, const QString &s);
  QString toSerializedString() const;

  value_t converted(type_t to, bool *ok = nullptr) const;

  typedef component_t(*Converter_t)(const component_t &v);
  static void registerConverter(const type_t &from, const type_t &to, Converter_t converter);
  static void registerDefaultConverters();

  static const std::vector<type_t> XYZW_IDS;
  static const std::vector<type_t> RGBA_IDS;

private:
  type_t type_;
  std::vector<component_t> data_;
  component_t meta_;

  static std::map<type_t, std::map<type_t, Converter_t> > converters_;

};

}

Q_DECLARE_METATYPE(olive::value_t)

#endif // NODEVALUE_H
