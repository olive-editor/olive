#ifndef DOUBLEFIELD_H
#define DOUBLEFIELD_H

#include "effectfield.h"
#include "ui/labelslider.h"

class DoubleField : public EffectField
{
  Q_OBJECT
public:
  DoubleField(EffectRow* parent, const QString& id);

  /**
   * @brief Get double value at timecode
   *
   * Convenience function. Equivalent to GetValueAt().toDouble()
   *
   * @param timecode
   *
   * Timecode to retrieve value at
   *
   * @return
   *
   * Double value at the set timecode
   */
  double GetDoubleAt(double timecode);

  void SetMinimum(double minimum);
  void SetMaximum(double maximum);
  void SetDefault(double maximum);

  void SetDisplayType(LabelSlider::DisplayType type);
  void SetFrameRate(const double& rate);

  virtual QVariant ConvertStringToValue(const QString& s);
  virtual QString ConvertValueToString(const QVariant& v);
private:
  double min_;
  double max_;
  double default_;

  LabelSlider::DisplayType display_type_;
  double frame_rate_;

  bool value_set_;
private slots:
  void ValueHasBeenSet();
};

#endif // DOUBLEFIELD_H
