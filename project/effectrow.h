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

#ifndef EFFECTROW_H
#define EFFECTROW_H

#include <QObject>
#include <QVector>

class Effect;
class QGridLayout;
class EffectField;
class QLabel;
class QPushButton;
class ComboAction;
class QHBoxLayout;
class KeyframeNavigator;
class ClickableLabel;

#include "effectfields.h"

class EffectRow : public QObject {
  Q_OBJECT
public:
  EffectRow(Effect* parent, const QString& n, bool savable = true, bool keyframable = true);

  void AddField(EffectField* Field);

  EffectField* Field(int i);
  int FieldCount();
  void set_keyframe_now(ComboAction *ca);
  void delete_keyframe_at_time(ComboAction *ca, long time);

  Effect* GetParentEffect();

  const QString& name();

  bool IsKeyframing();
  void SetKeyframing(bool);

  bool IsSavable();
public slots:
  void GoToPreviousKeyframe();
  void ToggleKeyframe();
  void GoToNextKeyframe();
  void FocusRow();
signals:
  void KeyframingSetChanged(bool);
private slots:
  void SetKeyframingEnabled(bool);
private:
  QString name_;

  bool keyframable_;
  bool keyframing_;
  bool savable_;

  QVector<EffectField*> fields_;
};

#endif // EFFECTROW_H
