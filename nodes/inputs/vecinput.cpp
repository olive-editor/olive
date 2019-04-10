#include "vecinput.h"

#include <QVector3D>
#include <QVector4D>
#include <QDebug>

VecInput::VecInput(Effect* parent, const QString& id, const QString& name, int values, bool savable, bool keyframable) :
  EffectRow(parent, id, name, savable, keyframable),
  single_value_mode_(false),
  values_(values)
{
  Q_ASSERT(values_ >= 1 && values_ <= 4);

  for (int i=0;i<values_;i++) {
    AddField(new DoubleField(this));
  }
}

void VecInput::SetMinimum(double minimum)
{
  for (int i=0;i<values_;i++) {
    static_cast<DoubleField*>(Field(i))->SetMinimum(minimum);
  }
}

void VecInput::SetMaximum(double maximum)
{
  for (int i=0;i<values_;i++) {
    static_cast<DoubleField*>(Field(i))->SetMaximum(maximum);
  }
}

void VecInput::SetDefault(double def)
{
  for (int i=0;i<values_;i++) {
    static_cast<DoubleField*>(Field(i))->SetDefault(def);
  }
}

void VecInput::SetDefault(const QVector<double> &def)
{
  for (int i=0;i<values_;i++) {
    static_cast<DoubleField*>(Field(i))->SetDefault(def.at(i));
  }
}

void VecInput::SetDisplayType(LabelSlider::DisplayType type)
{
  for (int i=0;i<FieldCount();i++) {
    static_cast<DoubleField*>(Field(i))->SetDisplayType(type);
  }
}

void VecInput::SetFrameRate(const double &rate)
{
  for (int i=0;i<FieldCount();i++) {
    static_cast<DoubleField*>(Field(i))->SetFrameRate(rate);
  }
}

void VecInput::SetSingleValueMode(bool on)
{
  qDebug() << "i'm here";

  single_value_mode_ = on;

  // Disable all fields except the first
  for (int i=1;i<FieldCount();i++) {
    Field(i)->SetEnabled(!single_value_mode_);
  }
}

DoubleInput::DoubleInput(Effect *parent, const QString &id, const QString &name, bool savable, bool keyframable) :
  VecInput(parent, id, name, 1, savable, keyframable)
{
}

double DoubleInput::GetDoubleAt(double timecode)
{
  return static_cast<DoubleField*>(Field(0))->GetDoubleAt(timecode);
}

Vec2Input::Vec2Input(Effect *parent, const QString &id, const QString &name, bool savable, bool keyframable) :
  VecInput(parent, id, name, 2, savable, keyframable)
{
}

QVector2D Vec2Input::GetVector2DAt(double timecode)
{
  return GetValueAt(timecode).value<QVector2D>();
}

QVariant Vec2Input::GetValueAt(double timecode)
{
  QVector2D vec2;
  vec2.setX(static_cast<DoubleField*>(Field(0))->GetDoubleAt(timecode));

  if (single_value_mode_) {
    vec2.setY(static_cast<DoubleField*>(Field(0))->GetDoubleAt(timecode));
  } else {
    vec2.setY(static_cast<DoubleField*>(Field(1))->GetDoubleAt(timecode));
  }

  return vec2;
}

void Vec2Input::SetValueAt(double timecode, const QVariant &value)
{
  QVector2D vec2 = value.value<QVector2D>();

  static_cast<DoubleField*>(Field(0))->SetValueAt(timecode, vec2.x());
  static_cast<DoubleField*>(Field(1))->SetValueAt(timecode, vec2.y());
}

Vec3Input::Vec3Input(Effect *parent, const QString &id, const QString &name, bool savable, bool keyframable) :
  VecInput(parent, id, name, 3, savable, keyframable)
{
}

QVector3D Vec3Input::GetVector3DAt(double timecode)
{
  return GetValueAt(timecode).value<QVector3D>();
}

QVariant Vec3Input::GetValueAt(double timecode)
{
  QVector3D vec3;
  vec3.setX(static_cast<DoubleField*>(Field(0))->GetDoubleAt(timecode));

  if (single_value_mode_) {
    vec3.setY(static_cast<DoubleField*>(Field(0))->GetDoubleAt(timecode));
    vec3.setZ(static_cast<DoubleField*>(Field(0))->GetDoubleAt(timecode));
  } else {
    vec3.setY(static_cast<DoubleField*>(Field(1))->GetDoubleAt(timecode));
    vec3.setZ(static_cast<DoubleField*>(Field(2))->GetDoubleAt(timecode));
  }

  return vec3;
}

void Vec3Input::SetValueAt(double timecode, const QVariant &value)
{
  QVector3D vec3 = value.value<QVector3D>();

  static_cast<DoubleField*>(Field(0))->SetValueAt(timecode, vec3.x());
  static_cast<DoubleField*>(Field(1))->SetValueAt(timecode, vec3.y());
  static_cast<DoubleField*>(Field(2))->SetValueAt(timecode, vec3.z());
}

Vec4Input::Vec4Input(Effect *parent, const QString &id, const QString &name, bool savable, bool keyframable) :
  VecInput(parent, id, name, 4, savable, keyframable)
{
}

QVector4D Vec4Input::GetVector4DAt(double timecode)
{
  return GetValueAt(timecode).value<QVector4D>();
}

QVariant Vec4Input::GetValueAt(double timecode)
{
  QVector4D vec4;
  vec4.setX(static_cast<DoubleField*>(Field(0))->GetDoubleAt(timecode));

  if (single_value_mode_) {
    vec4.setY(static_cast<DoubleField*>(Field(0))->GetDoubleAt(timecode));
    vec4.setZ(static_cast<DoubleField*>(Field(0))->GetDoubleAt(timecode));
    vec4.setZ(static_cast<DoubleField*>(Field(0))->GetDoubleAt(timecode));
  } else {
    vec4.setY(static_cast<DoubleField*>(Field(1))->GetDoubleAt(timecode));
    vec4.setZ(static_cast<DoubleField*>(Field(2))->GetDoubleAt(timecode));
    vec4.setW(static_cast<DoubleField*>(Field(3))->GetDoubleAt(timecode));
  }

  return vec4;
}

void Vec4Input::SetValueAt(double timecode, const QVariant &value)
{
  QVector4D vec4 = value.value<QVector4D>();

  static_cast<DoubleField*>(Field(0))->SetValueAt(timecode, vec4.x());
  static_cast<DoubleField*>(Field(1))->SetValueAt(timecode, vec4.y());
  static_cast<DoubleField*>(Field(2))->SetValueAt(timecode, vec4.z());
  static_cast<DoubleField*>(Field(3))->SetValueAt(timecode, vec4.w());
}
