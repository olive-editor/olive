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
  EffectRow(Effect* parent, bool save, QGridLayout* uilayout, const QString& n, int row, bool keyframable = true);
  ~EffectRow();

  void AddField(EffectField* field);

  void add_widget(QWidget *w);
  EffectField* field(int i);
  int fieldCount();
  void set_keyframe_now(ComboAction *ca);
  void delete_keyframe_at_time(ComboAction *ca, long time);
  ClickableLabel* label;
  Effect* parent_effect;
  bool savable;
  const QString& name();

  bool isKeyframing();
  void setKeyframing(bool);
public slots:
  void GoToPreviousKeyframe();
  void ToggleKeyframe();
  void GoToNextKeyframe();
  void FocusRow();
private slots:
  void SetKeyframingEnabled(bool);
private:
  bool keyframing;
  QGridLayout* ui;
  QString name_;
  int ui_row;
  QVector<EffectField*> fields_;
  QVector<QWidget*> widgets;

  KeyframeNavigator* keyframe_nav;

  bool just_made_unsafe_keyframe;
  QVector<int> unsafe_keys;
  QVector<QVariant> unsafe_old_data;
  QVector<bool> key_is_new;

  int column_count;
};

#endif // EFFECTROW_H
