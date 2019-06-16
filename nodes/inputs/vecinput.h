#ifndef VEC2INPUT_H
#define VEC2INPUT_H

#include <QVector2D>

#include "nodes/nodeio.h"

class VecInput : public NodeIO
{
  Q_OBJECT
public:
  VecInput(Node* parent, const QString& id, const QString& name, int values, bool savable = true, bool keyframable = true);

  void SetMinimum(double minimum);
  void SetMaximum(double maximum);
  void SetDefault(double def);

  void SetDefault(const QVector<double> &def);

  /**
   * @brief Sets the UI display type to a member of LabelSlider::DisplayType.
   */
  void SetDisplayType(LabelSlider::DisplayType type);

  /**
   * @brief For a timecode-based display type, sets the frame rate to be used for the displayed timecode
   *
   * \see SetDisplayType() and LabelSlider::SetFrameRate().
   */
  void SetFrameRate(const double& rate);

protected:
  bool single_value_mode_;

public slots:
  void SetSingleValueMode(bool on);

private:

  int values_;
};

class DoubleInput : public VecInput
{
public:
  DoubleInput(Node* parent, const QString& id, const QString& name, bool savable = true, bool keyframable = true);

  double GetDoubleAt(const rational &timecode);
};

class Vec2Input : public VecInput {
public:
  Vec2Input(Node* parent, const QString& id, const QString& name, bool savable = true, bool keyframable = true);

  QVector2D GetVector2DAt(const rational& timecode);
  virtual QVariant GetValueAt(const rational& timecode) override;
  virtual void SetValueAt(const rational& timecode, const QVariant &value) override;
};

class Vec3Input : public VecInput {
public:
  Vec3Input(Node* parent, const QString& id, const QString& name, bool savable = true, bool keyframable = true);

  QVector3D GetVector3DAt(const rational& timecode);
  virtual QVariant GetValueAt(const rational& timecode) override;
  virtual void SetValueAt(const rational& timecode, const QVariant &value) override;
};

class Vec4Input : public VecInput {
public:
  Vec4Input(Node* parent, const QString& id, const QString& name, bool savable = true, bool keyframable = true);

  QVector4D GetVector4DAt(const rational& timecode);
  virtual QVariant GetValueAt(const rational& timecode) override;
  virtual void SetValueAt(const rational& timecode, const QVariant &value) override;
};

#endif // VEC2INPUT_H
