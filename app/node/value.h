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

#include <QMatrix4x4>
#include <QString>
#include <QVariant>
#include <QVector>

#include "common/qtutils.h"
#include "node/splitvalue.h"
#include "render/texture.h"

namespace olive {

class Node;
class NodeValue;
class SampleJob;

using NodeValueArray = std::vector<NodeValue>;

class NodeValue
{
public:
  /**
   * @brief The types of data that can be passed between Nodes
   */
  enum Type {
    kNone,

    /**
     ****************************** SPECIFIC IDENTIFIERS ******************************
     */

    /**
     * Integer type
     *
     * Resolves to int64_t.
     */
    kInt,

    /**
     * Decimal (floating-point) type
     *
     * Resolves to `double`.
     */
    kFloat,

    /**
     * Decimal (rational) type
     *
     * Resolves to `double`.
     */
    kRational,

    /**
     * Boolean type
     *
     * Resolves to `bool`.
     */
    kBoolean,

    /**
     * Floating-point type
     *
     * Resolves to `Color`.
     *
     * Colors passed around the nodes should always be in reference space and preferably use
     */
    kColor,

    /**
     * Matrix type
     *
     * Resolves to `QMatrix4x4`.
     */
    kMatrix,

    /**
     * Text type
     *
     * Resolves to `QString`.
     */
    kText,

    /**
     * Font type
     *
     * Resolves to `QFont`.
     */
    kFont,

    /**
     * File type
     *
     * Resolves to a `QString` containing an absolute file path.
     */
    kFile,

    /**
     * Image buffer type
     *
     * True value type depends on the render engine used.
     */
    kTexture,

    /**
     * Audio samples type
     *
     * Resolves to `SampleBufferPtr`.
     */
    kSamples,

    /**
     * Two-dimensional vector (XY) type
     *
     * Resolves to `QVector2D`.
     */
    kVec2,

    /**
     * Three-dimensional vector (XYZ) type
     *
     * Resolves to `QVector3D`.
     */
    kVec3,

    /**
     * Four-dimensional vector (XYZW) type
     *
     * Resolves to `QVector4D`.
     */
    kVec4,

    /**
     * Cubic bezier type that contains three X/Y coordinates, the main point, and two control points
     *
     * Resolves to `Bezier`
     */
    kBezier,

    /**
     * ComboBox type
     *
     * Resolves to `int` - the index currently selected
     */
    kCombo,

    /**
     * Video Parameters type
     *
     * Resolves to `VideoParams`
     */
    kVideoParams,

    /**
     * Audio Parameters type
     *
     * Resolves to `AudioParams`
     */
    kAudioParams,

    /**
     * Subtitle Parameters type
     *
     * Resolves to `SubtitleParams`
     */
    kSubtitleParams,

    /**
     * Binary Data
     */
    kBinary,

    /**
     * End of list
     */
    kDataTypeCount
  };

  template <typename T>
  NodeValue(Type type, const T& data)
  {
    type_ = type;
    set_value(data);
    array_ = false;
  }

  NodeValue(Type type, const NodeValueArray &array)
  {
    type_ = type;
    set_value(array);
    array_ = true;
  }

  NodeValue() :
    NodeValue(kNone, QVariant())
  {
  }

  NodeValue(const QMatrix4x4& data) :
    NodeValue(NodeValue::kMatrix, data)
  {
  }

  NodeValue(TexturePtr texture) :
    NodeValue(NodeValue::kTexture, texture)
  {
  }

  NodeValue(const SampleJob &samples);

  NodeValue(const SampleBuffer &samples) :
    NodeValue(NodeValue::kSamples, samples)
  {
  }

  NodeValue(const QVector2D &vec) :
    NodeValue(NodeValue::kVec2, vec)
  {
  }

  NodeValue(const QVector3D &vec) :
    NodeValue(NodeValue::kVec3, vec)
  {
  }

  NodeValue(const QVector4D &vec) :
    NodeValue(NodeValue::kVec4, vec)
  {
  }

  NodeValue(const double &d) :
    NodeValue(NodeValue::kFloat, d)
  {
  }

  NodeValue(const int64_t &i) :
    NodeValue(NodeValue::kInt, i)
  {
  }

  NodeValue(const int &i) :
    NodeValue(NodeValue::kInt, i)
  {
  }

  NodeValue(const bool &i) :
    NodeValue(NodeValue::kBoolean, i)
  {
  }

  NodeValue(const Color &i) :
    NodeValue(NodeValue::kColor, i)
  {
  }

  NodeValue(const QString &i) :
    NodeValue(NodeValue::kText, i)
  {
  }

  NodeValue(const rational &i) :
    NodeValue(NodeValue::kRational, i)
  {
  }

  Type type() const
  {
    return type_;
  }

  template <typename T>
  T value() const
  {
    return data_.value<T>();
  }

  template <typename T>
  void set_value(const T &v)
  {
    data_ = QVariant::fromValue(v);
  }

  const QVariant &data() const { return data_; }

  template <typename T>
  bool canConvert() const
  {
    return data_.canConvert<T>();
  }

  bool array() const
  {
    return array_;
  }

  bool operator==(const NodeValue& rhs) const
  {
    return type_ == rhs.type_ && data_ == rhs.data_;
  }

  operator bool() const
  {
    return !data_.isNull();
  }

  static QString GetPrettyDataTypeName(Type type);

  static QString GetDataTypeName(Type type);
  static NodeValue::Type GetDataTypeFromName(const QString &n);

  static QString ValueToString(Type data_type, const QVariant& value, bool value_is_a_key_track);
  static QString ValueToString(const NodeValue &v, bool value_is_a_key_track)
  {
    return ValueToString(v.type_, v.data_, value_is_a_key_track);
  }

  static QVariant StringToValue(Type data_type, const QString &string, bool value_is_a_key_track);

  static QVector<QVariant> split_normal_value_into_track_values(Type type, const QVariant &value);

  static QVariant combine_track_values_into_normal_value(Type type, const QVector<QVariant>& split);

  SplitValue to_split_value() const
  {
    return split_normal_value_into_track_values(type_, data_);
  }

  /**
   * @brief Returns whether a data type can be interpolated or not
   */
  static bool type_can_be_interpolated(NodeValue::Type type)
  {
    return type == kFloat
        || type == kVec2
        || type == kVec3
        || type == kVec4
        || type == kBezier
        || type == kColor
        || type == kRational;
  }

  static bool type_is_numeric(NodeValue::Type type)
  {
    return type == kFloat
        || type == kInt
        || type == kRational;
  }

  static bool type_is_vector(NodeValue::Type type)
  {
    return type == kVec2
        || type == kVec3
        || type == kVec4;
  }

  static bool type_is_buffer(NodeValue::Type type)
  {
    return type == kTexture
        || type == kSamples;
  }

  static int get_number_of_keyframe_tracks(Type type);

  static void ValidateVectorString(QStringList* list, int count);

  TexturePtr toTexture() const { return value<TexturePtr>(); }
  SampleBuffer toSamples() const { return value<SampleBuffer>(); }
  bool toBool() const { return value<bool>(); }
  double toDouble() const { return value<double>(); }
  int64_t toInt() const { return value<int64_t>(); }
  rational toRational() const { return value<olive::core::rational>(); }
  QString toString() const { return value<QString>(); }
  Color toColor() const { return value<olive::core::Color>(); }
  QMatrix4x4 toMatrix() const { return value<QMatrix4x4>(); }
  VideoParams toVideoParams() const { return value<VideoParams>(); }
  AudioParams toAudioParams() const { return value<AudioParams>(); }
  QVector2D toVec2() const { return value<QVector2D>(); }
  QVector3D toVec3() const { return value<QVector3D>(); }
  QVector4D toVec4() const { return value<QVector4D>(); }
  Bezier toBezier() const { return value<Bezier>(); }
  NodeValueArray toArray() const { return value<NodeValueArray>(); }

private:
  Type type_;
  QVariant data_;
  bool array_;

};

}

Q_DECLARE_METATYPE(olive::NodeValue)

#endif // NODEVALUE_H
