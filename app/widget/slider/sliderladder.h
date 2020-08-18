/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2019 Olive Team

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

#ifndef SLIDERLADDER_H
#define SLIDERLADDER_H

#include <QLabel>
#include <QTimer>
#include <QWidget>

#include "common/define.h"

OLIVE_NAMESPACE_ENTER

class SliderLadderElement : public QWidget
{
  Q_OBJECT
public:
  SliderLadderElement(const double& multiplier, QWidget* parent = nullptr);

  void SetHighlighted(bool e);

  void SetValue(const QString& value);

  void SetMultiplierVisible(bool e);

  double GetMultiplier() const
  {
    return multiplier_;
  }

private:
  void UpdateLabel();

  QLabel* label_;

  double multiplier_;
  QString value_;

  bool highlighted_;

  bool multiplier_visible_;

};

class SliderLadder : public QFrame
{
  Q_OBJECT
public:
  SliderLadder(double drag_multiplier, int nb_outer_values, QWidget* parent = nullptr);

  virtual ~SliderLadder() override;

  void SetValue(const QString& s);

protected:
  virtual void mouseReleaseEvent(QMouseEvent *event) override;

  virtual void showEvent(QShowEvent *event) override;

signals:
  void DraggedByValue(int value, double multiplier);

  void Released();

private:
  QPoint drag_start_;

  QList<SliderLadderElement*> elements_;

  int active_element_;

  QTimer drag_timer_;

  int y_mobility_;

private slots:
  void TimerUpdate();

};

OLIVE_NAMESPACE_EXIT

#endif // SLIDERLADDER_H
