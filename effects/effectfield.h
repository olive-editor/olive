/***

    Olive - Non-Linear Video Editor
    Copyright (C) 2019  Olive Team

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

#ifndef EFFECTFIELD_H
#define EFFECTFIELD_H

#include <QObject>
#include <QVariant>
#include <QVector>

#include "effects/keyframe.h"
#include "undo/undo.h"

class EffectRow;
class ComboAction;

class EffectField : public QObject {
  Q_OBJECT
public:
  enum EffectFieldType {
    EFFECT_FIELD_DOUBLE,
    EFFECT_FIELD_COLOR,
    EFFECT_FIELD_STRING,
    EFFECT_FIELD_BOOL,
    EFFECT_FIELD_COMBO,
    EFFECT_FIELD_FONT,
    EFFECT_FIELD_FILE,
    EFFECT_FIELD_UI
  };

  EffectField(EffectRow* parent, const QString& i, EffectFieldType t);
  ~EffectField();

  EffectRow* GetParentRow();

  const EffectFieldType& type();
  const QString& id();

  QVariant GetValueAt(double timecode);
  void SetValueAt(double time, const QVariant& value);

  double Now();
  long NowInFrames();

  void PrepareDataForKeyframing(bool enabled, ComboAction* ca);

  int GetColumnSpan();
  void SetColumnSpan(int i);

  virtual QVariant ConvertStringToValue(const QString& s);
  virtual QString ConvertValueToString(const QVariant& v);

  virtual QWidget* CreateWidget();
  virtual void UpdateWidgetValue(QWidget* widget, double);

  double GetValidKeyframeHandlePosition(int key, bool post);

  bool IsEnabled();
  void SetEnabled(bool e);

  QVariant persistent_data_;
  QVector<EffectKeyframe> keyframes;

signals:
  void Changed();
  void Clicked();

  void EnabledChanged(bool);

private:
  EffectFieldType type_;
  QString id_;

  bool HasKeyframes();
  double frameToTimecode(long frame);
  long timecodeToFrame(double timecode);
  void get_keyframe_data(double timecode, int& before, int& after, double& d);

  bool enabled_;
  int colspan_;

};

#endif // EFFECTFIELD_H
